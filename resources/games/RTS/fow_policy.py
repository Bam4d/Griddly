"""Scripted AI player for GriddlyRTS-FoW ("Griddly RTS: Frontiers").

The GDY holds all of the rules; this module only decides what to do. Every tick it reads the game
state and issues at most one external action per unit (plus camera pans). The same logic is ported
to TypeScript for the web UI (studio/griddly-demo/apps/web/src/rts/ai.ts), so keep the two in step.

Plan, per player:
    economy    train workers (up to 6); each worker mines the nearest free mineral / gem slot,
               carries ONE load back to the nearest friendly base and deposits it; when nothing is
               left to mine, workers chop forest next to the base for wood
    expand     a scout (or idle soldier) walks to the nearest uncaptured relay and captures it
    build      lab -> barracks -> towers near the base, facing the enemy
    research   along the player's strategy (see STRATEGIES)
    army       the barracks trains the strategy's units; medics follow and heal; the army gathers
               at a rally point and attacks once it is big enough (or the economy runs dry)

Fog of war: a policy only ever reads its own player's view of the game, ``game.get_player_state(pid)``
(the engine filters it by Environment.Player.FogOfWar): its own objects, terrain it has explored, and
enemy objects only while they are in sight. It remembers the enemy buildings it has seen, guesses the
enemy base from the map's point symmetry until it finds it, and sends its scout to explore.

`Match` runs two policies against each other on the engine's game clock: each records its actions
from its own player state (``player.queue``), and every recorded action runs together on the tick.
"""
import random
from collections import deque

LEFT, UP, RIGHT, DOWN = 1, 2, 3, 4
DIRS = {LEFT: (-1, 0), UP: (0, -1), RIGHT: (1, 0), DOWN: (0, 1)}

COSTS = {"worker": 5, "scout": 4, "combat": 5, "marine": 6, "medic": 6, "tank": 12,
         "barracks": 15, "lab": 10, "tower": 12, "wall": 2}
UNITS = ("worker", "scout", "combat", "marine", "medic", "tank")
ARMY = ("scout", "combat", "marine", "medic", "tank")
FIGHTERS = ("combat", "marine", "tank")
BUILDINGS = ("base", "barracks", "lab", "tower", "wall", "relay_owned")
WALKABLE = (None, "bridge", "map_size")
DISPLAY = {"combat": "trooper", "marine": "gunner", "tank": "tank", "worker": "worker", "scout": "scout",
           "medic": "medic", "base": "base", "barracks": "barracks", "lab": "lab", "tower": "tower",
           "wall": "wall", "relay_owned": "relay"}

# research tree: tech id -> (global variable, caption name, cost, prerequisite variable)
TECHS = {
    1: ("res_mining", "Mining", 8, None),
    2: ("res_weapons", "Weapons", 10, None),
    3: ("res_armor", "Armour", 10, None),
    4: ("res_gunnery", "Gunnery", 12, "res_weapons"),
    5: ("res_vehicles", "Vehicles", 12, "res_armor"),
    6: ("res_fortify", "Fortify", 10, "res_armor"),
}
TECH_VARS = [v[0] for v in TECHS.values()]
UNIT_NEEDS = {"marine": "res_gunnery", "tank": "res_vehicles"}

# each player's research path and army mix (unit, share of the army)
STRATEGIES = {
    1: {"name": "GUNLINE", "research": [1, 2, 4, 3], "army": [("marine", 3), ("combat", 2), ("medic", 1)],
        "towers": 1},
    2: {"name": "ARMOUR", "research": [1, 3, 5, 6], "army": [("tank", 2), ("combat", 3), ("medic", 1)],
        "towers": 2},
}

TARGET_WORKERS = 6
ATTACK_SIZE = 7
TARGET_PRIORITY = {"marine": 0, "medic": 1, "combat": 2, "tank": 3, "scout": 4, "worker": 5,
                   "tower": 6, "barracks": 7, "lab": 8, "wall": 10, "base": 11}
CAMERA_PANS_PER_TICK = 2


def direction_to(src, dst):
    d = (dst[0] - src[0], dst[1] - src[1])
    for k, v in DIRS.items():
        if v == d:
            return k
    return None


def neighbours(c):
    return [(c[0] + dx, c[1] + dy) for dx, dy in DIRS.values()]


def manhattan(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1])


UNSEEN, EXPLORED, VISIBLE = 0, 1, 2


class WorldView:
    """One player's view of the game for one tick: ``game.get_player_state(pid)``.

    ``visibility`` is the player's fog (row major, UNSEEN / EXPLORED / VISIBLE); None without fog.
    Global variables come from the state, which only holds the viewing player's per-player values.
    """

    def __init__(self, state, width, height):
        self.width, self.height = width, height
        self.tick = state["GameTicks"]
        self.globals = state["GlobalVariables"]
        self.visibility = state.get("Visibility")
        self.objects = []
        self.at = {}
        for o in state["Objects"]:
            x, y = o["Location"]
            if x < 0 or y < 0 or o["Name"] == "map_size":
                continue
            obj = {"name": o["Name"], "pos": (x, y), "player": o["PlayerId"], "vars": o.get("Variables", {}) or {}}
            self.objects.append(obj)
            # the highest object on a cell wins (units on bridges)
            if (x, y) not in self.at or obj["name"] != "bridge":
                self.at[(x, y)] = obj
        self.by_name = {}
        for o in self.objects:
            self.by_name.setdefault(o["name"], []).append(o)

    def of(self, name, player=None):
        return [o for o in self.by_name.get(name, []) if player is None or o["player"] == player]

    def global_value(self, name, player):
        values = self.globals.get(name, [])
        return int(values[player]) if player < len(values) else 0

    def resources(self, player):
        return self.global_value("player_resources", player)

    def tech(self, var, player):
        return self.global_value(var, player)

    def camera(self, player):
        return self.global_value("cam_x", player), self.global_value("cam_y", player)

    def visible(self, c):
        """Does the viewing player see cell c right now?"""
        return self.visibility is None or (self.inside(c) and self.visibility[c[1] * self.width + c[0]] == VISIBLE)

    def unseen(self, c):
        return self.visibility is not None and self.inside(c) and self.visibility[c[1] * self.width + c[0]] == UNSEEN

    def inside(self, c):
        return 0 <= c[0] < self.width and 0 <= c[1] < self.height

    def blocked(self, c):
        o = self.at.get(c)
        return o is not None and o["name"] not in ("bridge",)


class Planner:
    """Per-tick occupancy bookkeeping so a player's own units never plan into each other."""

    def __init__(self, world):
        self.world = world
        self.blocked = {c for c in world.at if world.blocked(c)}
        self.reserved = set()

    def free(self, c):
        return self.world.inside(c) and c not in self.blocked

    def move(self, src, dst):
        self.blocked.discard(src)
        self.blocked.add(dst)

    def spawn(self, c):
        self.blocked.add(c)

    def bfs(self, start, goals, ignore_units=False, limit=4000):
        """First step of a shortest path from start to any goal: (direction, length, goal) or None."""
        goals = set(goals)
        if not goals:
            return None
        if start in goals:
            return None, 0, start
        static = {"forest", "rock", "water", "minerals", "gems", "relay", "base", "barracks", "lab", "tower",
                  "wall", "relay_owned"}

        def passable(c):
            if not self.world.inside(c):
                return False
            if ignore_units:
                o = self.world.at.get(c)
                return o is None or o["name"] not in static
            return c not in self.blocked or (c in goals and False)

        prev = {start: None}
        q = deque([start])
        while q and len(prev) < limit:
            cur = q.popleft()
            if cur in goals:
                path = [cur]
                while prev[path[-1]] != start:
                    path.append(prev[path[-1]])
                return direction_to(start, path[-1]), len(path), cur
            for n in neighbours(cur):
                if n not in prev and (passable(n) or (n in goals and n not in self.blocked)):
                    prev[n] = cur
                    q.append(n)
        return None


class RTSPolicy:
    """One player's strategy. ``act(world)`` returns ([(action, x, y, actionId)], caption_lines)."""

    def __init__(self, player_id, enemy_id, seed=0):
        self.pid = player_id
        self.enemy = enemy_id
        self.rng = random.Random(seed * 1000 + player_id)
        self.strategy = STRATEGIES[player_id]
        self.offensive = False
        self.counts = {}
        self.events = []
        self._last_tech = {}
        self.sites = {}  # building -> chosen site, kept until built or blocked by terrain/buildings
        self.known_enemy = {}  # pos -> name of enemy buildings seen, until seen gone
        self.explore_goals = {}  # unit position-independent: one goal per role label

    # ------------------------------------------------------------------ helpers
    def _emit(self, actions, name, pos, action_id, label):
        actions.append((name, pos[0], pos[1], action_id))
        self.counts[label] = self.counts.get(label, 0) + 1

    def _walk(self, plan, actions, unit, goals, label, allow_chop=False):
        """One step towards any goal cell; sidestep randomly when the way is jammed by units."""
        pos = unit["pos"]
        step = plan.bfs(pos, goals)
        if step is None:
            step = plan.bfs(pos, goals, ignore_units=True)
        if step is None or step[0] is None:
            return False
        d = step[0]
        nxt = (pos[0] + DIRS[d][0], pos[1] + DIRS[d][1])
        if not plan.free(nxt):
            options = [k for k in DIRS if plan.free((pos[0] + DIRS[k][0], pos[1] + DIRS[k][1]))]
            if not options or self.rng.random() < 0.5:
                return False
            d = self.rng.choice(options)
            nxt = (pos[0] + DIRS[d][0], pos[1] + DIRS[d][1])
        plan.move(pos, nxt)
        self._emit(actions, "move", pos, d, label)
        unit["pos"] = nxt
        return True

    def _site(self, world, plan, base, towards, dmin, dmax, avoid):
        """A free tile dmin..dmax steps from `base`, not touching resources/buildings, closest to `towards`."""
        best = None
        for y in range(max(0, base[1] - dmax), min(world.height, base[1] + dmax + 1)):
            for x in range(max(0, base[0] - dmax), min(world.width, base[0] + dmax + 1)):
                c = (x, y)
                if not dmin <= manhattan(c, base) <= dmax or not plan.free(c) or c in plan.reserved:
                    continue
                if world.at.get(c) is not None:
                    continue
                if any(world.at.get(n) is not None and world.at[n]["name"] in avoid for n in neighbours(c)):
                    continue
                if sum(plan.free(n) for n in neighbours(c)) < 3:
                    continue
                score = (manhattan(c, towards), x, y)
                if best is None or score < best[0]:
                    best = (score, c)
        return best[1] if best else None

    def _construct(self, world, plan, actions, workers, site, building, res):
        """Send the nearest idle worker next to `site` and build there. Returns (builder, res)."""
        free_workers = [w for w in workers if not w["vars"].get("carrying")]
        if not site or not free_workers:
            return None, res
        builder = min(free_workers, key=lambda w: manhattan(w["pos"], site))
        d = direction_to(builder["pos"], site)
        if d is not None:
            if res >= COSTS[building] and world.at.get(site) is None:
                self._emit(actions, f"build_{building}", builder["pos"], d, f"BUILD {building.upper()}")
                plan.spawn(site)
                res -= COSTS[building]
            plan.reserved.add(builder["pos"])
        else:
            goals = [n for n in neighbours(site) if plan.free(n) or n == builder["pos"]]
            self._walk(plan, actions, builder, goals, f"walk to {building} site")
        return builder, res

    def _next_research(self, world):
        for tid in self.strategy["research"]:
            if world.tech(TECHS[tid][0], self.pid) != 2:
                return tid
        return None

    def _unit_choice(self, world, army):
        """The unit of the strategy's mix the army is most short of, among those researched."""
        counts = {}
        for u in army:
            counts[u["name"]] = counts.get(u["name"], 0) + 1
        best = None
        for unit, share in self.strategy["army"]:
            needs = UNIT_NEEDS.get(unit)
            if needs and world.tech(needs, self.pid) != 2:
                continue
            ratio = counts.get(unit, 0) / share
            if best is None or ratio < best[0]:
                best = (ratio, unit)
        return best[1] if best else "combat"

    def _remember(self, world):
        """Keep the enemy buildings we have seen; forget them once we see their cell empty."""
        for o in world.objects:
            if o["player"] == self.enemy and o["name"] in BUILDINGS:
                self.known_enemy[o["pos"]] = o["name"]
        for pos in list(self.known_enemy):
            o = world.at.get(pos)
            if world.visible(pos) and (o is None or o["player"] != self.enemy):
                del self.known_enemy[pos]

    def _enemy_base_guess(self, world, base):
        """Where the enemy base is: seen, remembered, or the mirror of ours (the maps are point-symmetric)."""
        seen = world.of("base", self.enemy)
        if seen:
            return seen[0]["pos"]
        known = [p for p, n in self.known_enemy.items() if n == "base"]
        if known:
            return known[0]
        return (world.width - 1 - base[0], world.height - 1 - base[1])

    def _explore(self, world, plan, actions, u, label="explore"):
        """Walk towards the nearest reachable unexplored cell (sampled every 3 cells)."""
        goal = self.explore_goals.get(label)
        if goal is None or not world.unseen(goal) or manhattan(u["pos"], goal) <= 2 or self.rng.random() < 0.02:
            cells = [(x, y) for y in range(1, world.height - 1, 3) for x in range(1, world.width - 1, 3) if world.unseen((x, y))]
            # unseen cells may be water or rock: walk to the nearest one a path reaches
            step = plan.bfs(u["pos"], cells, ignore_units=True, limit=8000) if cells else None
            goal = self.explore_goals[label] = step[2] if step else None
        if goal is not None:
            return self._walk(plan, actions, u, [goal] + neighbours(goal), label)
        return False

    # --------------------------------------------------------------------- policy
    def act(self, world):
        self.counts = {}
        plan = Planner(world)
        actions = []
        pid = self.pid
        res = world.resources(pid)
        bases = world.of("base", pid)
        base = bases[0]["pos"] if bases else None
        self._remember(world)
        # the enemy base as far as we know: seen, remembered, or guessed from the map's symmetry
        enemy_base = self._enemy_base_guess(world, base) if base else None
        mine = [o for o in world.objects if o["player"] == pid]
        workers = [o for o in mine if o["name"] == "worker"]
        army = [o for o in mine if o["name"] in ARMY]
        barracks = [o for o in mine if o["name"] == "barracks"]
        labs = [o for o in mine if o["name"] == "lab"]
        towers = [o for o in mine if o["name"] == "tower"]
        enemies = [o for o in world.objects if o["player"] == self.enemy and o["name"] in TARGET_PRIORITY]
        # remembered buildings that are out of sight right now are still worth marching to
        seen_pos = {o["pos"] for o in enemies}
        remembered = [{"name": n, "pos": p, "player": self.enemy, "vars": {}} for p, n in self.known_enemy.items()
                      if p not in seen_pos and n in TARGET_PRIORITY]
        enemy_at = {o["pos"]: o for o in enemies}
        resources = world.of("minerals") + world.of("gems")
        relays = [o for o in world.objects if o["name"] == "relay" or (o["name"] == "relay_owned" and o["player"] != pid)]
        self._track_research(world)
        next_tech = self._next_research(world)
        researching = any(world.tech(v, pid) == 1 for v in TECH_VARS)
        threat = [e for e in enemies if e["name"] in UNITS and base and manhattan(e["pos"], base) <= 8]
        if base is None:
            return actions, self._caption(world, "DEFEATED", res, workers, army)

        # ------------------------------------------------------------- base: workers
        if len(workers) < TARGET_WORKERS and res >= COSTS["worker"] and (len(workers) < 3 or labs):
            for d in (DOWN, RIGHT, UP, LEFT):
                c = (base[0] + DIRS[d][0], base[1] + DIRS[d][1])
                if plan.free(c):
                    self._emit(actions, "build_worker", base, d, "train worker")
                    plan.spawn(c)
                    res -= COSTS["worker"]
                    break

        # ---------------------------------------------------------------- buildings
        toward = enemy_base or (world.width // 2, world.height // 2)
        avoid = {"minerals", "gems", "base", "barracks", "lab", "tower"}
        builders = set()
        wanted = None
        if not labs and len(workers) >= 3:
            wanted = ("lab", self._site(world, plan, base, (base[0] * 2 - toward[0], base[1] * 2 - toward[1]), 2, 4, avoid))
        elif labs and not barracks and len(workers) >= 4:
            wanted = ("barracks", self._site(world, plan, base, toward, 3, 5, avoid))
        elif barracks and len(towers) < self.strategy["towers"] and (threat or len(army) >= 3):
            wanted = ("tower", self._site(world, plan, base, toward, 4, 6, avoid))
        if wanted:
            kind = wanted[0]
            site = self.sites.get(kind)
            occupant = world.at.get(site) if site else None
            if site is None or (occupant is not None and occupant["name"] not in UNITS):
                site = self.sites[kind] = wanted[1]
            wanted = (kind, site)
        if wanted and wanted[1]:
            # nobody else plans to stand on the building site
            plan.reserved.add(wanted[1])
        if wanted and wanted[1] and res >= COSTS[wanted[0]] - 3:
            builder, res = self._construct(world, plan, actions, workers, wanted[1], wanted[0], res)
            if builder:
                builders.add(id(builder))

        # ------------------------------------------------------------------ research
        if labs and next_tech is not None and not researching:
            var, label, cost, needs = TECHS[next_tech]
            if res >= cost and (needs is None or world.tech(needs, pid) == 2):
                self._emit(actions, "research", labs[0]["pos"], next_tech, f"RESEARCH {label.upper()}")
                res -= cost

        # ---------------------------------------------------------------------- army
        unit = self._unit_choice(world, army)
        keep = 0 if threat else (TECHS[next_tech][2] if next_tech and labs and not researching else 0)
        for b in barracks:
            if res - keep < COSTS[unit]:
                break
            spots = [(manhattan((b["pos"][0] + v[0], b["pos"][1] + v[1]), toward), k) for k, v in DIRS.items()
                     if plan.free((b["pos"][0] + v[0], b["pos"][1] + v[1]))]
            if spots:
                d = min(spots)[1]
                self._emit(actions, f"build_{unit}", b["pos"], d, f"train {DISPLAY[unit]}")
                plan.spawn((b["pos"][0] + DIRS[d][0], b["pos"][1] + DIRS[d][1]))
                res -= COSTS[unit]
        # a scout grabs relays and explores the fog (relays and minerals out of sight are unknown)
        unexplored = world.visibility is not None and world.visibility.count(UNSEEN) > len(world.visibility) // 3
        if barracks and not any(u["name"] == "scout" for u in army) and (relays or unexplored or not resources) \
                and res >= COSTS["scout"] + keep and world.tick < 900:
            b = barracks[0]["pos"]
            for d in (RIGHT, DOWN, LEFT, UP):
                c = (b[0] + DIRS[d][0], b[1] + DIRS[d][1])
                if plan.free(c):
                    self._emit(actions, "build_scout", b, d, "train scout")
                    plan.spawn(c)
                    res -= COSTS["scout"]
                    break

        # ------------------------------------------------------------------ workers
        self._workers(world, plan, actions, workers, builders, base, resources, enemy_at, threat)

        # --------------------------------------------------------------------- army
        fighters = [u for u in army if u["name"] in FIGHTERS]
        # attack with a full army, or with whatever we have once there is nothing known left to mine
        # (and no money to spend), or when the barracks is boxed in by idle units
        boxed_in = barracks and all(not plan.free(n) for b in barracks for n in neighbours(b["pos"]))
        if not self.offensive and (len(fighters) >= ATTACK_SIZE or (not resources and fighters and (res < 5 or boxed_in))
                                   or (boxed_in and len(fighters) >= 3)):
            self.offensive = True
        if self.offensive and len(fighters) < 2 and enemy_base and not threat:
            self.offensive = False  # regroup
        rally = (base[0] + (toward[0] > base[0]) * 6 - (toward[0] < base[0]) * 6,
                 base[1] + (toward[1] > base[1]) * 6 - (toward[1] < base[1]) * 6)
        rally = (min(max(rally[0], 1), world.width - 2), min(max(rally[1], 1), world.height - 2))
        rally = self._rally_point(world, base, rally)
        for u in sorted(army, key=lambda u: manhattan(u["pos"], toward)):
            if u["name"] == "scout":
                self._scout(world, plan, actions, u, relays, enemy_at)
            elif u["name"] == "medic":
                self._medic(world, plan, actions, u, army, rally)
            else:
                self._fight(world, plan, actions, u, enemies, enemy_at, enemy_base, rally, threat, remembered)

        # -------------------------------------------------------------------- camera
        focus = self._camera_focus(base, workers, fighters, enemy_base)
        cam = world.camera(pid)
        for _ in range(CAMERA_PANS_PER_TICK):
            dx, dy = focus[0] - cam[0], focus[1] - cam[1]
            if abs(dx) <= 1 and abs(dy) <= 1:
                break
            d = (RIGHT if dx > 0 else LEFT) if abs(dx) >= abs(dy) else (DOWN if dy > 0 else UP)
            actions.append(("move_camera", base[0], base[1], d))
            cam = (cam[0] + 2 * DIRS[d][0], cam[1] + 2 * DIRS[d][1])

        if self.offensive:
            phase = "ATTACK"
        elif threat:
            phase = "DEFEND"
        elif barracks:
            phase = "ARMY"
        elif labs:
            phase = "TECH"
        else:
            phase = "ECONOMY"
        return actions, self._caption(world, phase, world.resources(pid), workers, army)

    # ------------------------------------------------------------------ roles
    def _workers(self, world, plan, actions, workers, builders, base, resources, enemy_at, threat):
        res_cells = {r["pos"]: r for r in resources}
        # mine what is closer to us than to the enemy (or anything once ours is gone)
        eb = self._enemy_base_guess(world, base)
        ours = {p for p in res_cells if eb is None or manhattan(p, base) <= manhattan(p, eb) + 4}
        targets = ours or set(res_cells)
        drop = [n for n in neighbours(base)]
        order = sorted(workers, key=lambda w: (0 if w["vars"].get("carrying") else 1,
                                               min((manhattan(w["pos"], p) for p in targets), default=0)))
        for w in order:
            if id(w) in builders:
                continue
            pos = w["pos"]
            adjacent = [n for n in neighbours(pos) if n in enemy_at]
            if adjacent:
                t = min(adjacent, key=lambda c: TARGET_PRIORITY[enemy_at[c]["name"]])
                self._emit(actions, "attack", pos, direction_to(pos, t), "worker defends")
                continue
            if w["vars"].get("carrying"):
                if manhattan(pos, base) == 1:
                    self._emit(actions, "gather", pos, direction_to(pos, base), "deposit")
                else:
                    self._walk(plan, actions, w, drop, "carry")
                continue
            # repair damaged buildings when there is a threat
            damaged = [n for n in neighbours(pos) if world.at.get(n) and world.at[n]["player"] == self.pid
                       and world.at[n]["name"] in ("base", "barracks", "tower", "lab", "wall")
                       and world.at[n]["vars"].get("health", 0) < world.at[n]["vars"].get("max_health", 0)]
            if damaged and threat:
                self._emit(actions, "repair", pos, direction_to(pos, damaged[0]), "repair")
                continue
            near = [n for n in neighbours(pos) if n in targets]
            if near:
                gem = [n for n in near if res_cells[n]["name"] == "gems"]
                plan.reserved.add(pos)
                self._emit(actions, "gather", pos, direction_to(pos, (gem or near)[0]), "mine")
                continue
            slots = {n for p in targets for n in neighbours(p) if plan.free(n) and n not in plan.reserved}
            step = plan.bfs(pos, slots) if slots else None
            if step is not None:
                plan.reserved.add(step[2])
                self._walk(plan, actions, w, [step[2]], "to minerals")
                continue
            # nothing to mine: chop the nearest forest for wood
            trees = [n for n in neighbours(pos) if world.at.get(n) and world.at[n]["name"] == "forest"]
            if trees:
                self._emit(actions, "gather", pos, direction_to(pos, trees[0]), "chop wood")
                continue
            forest = [o["pos"] for o in world.of("forest") if manhattan(o["pos"], base) <= 12]
            slots = {n for p in forest for n in neighbours(p) if plan.free(n) and n not in plan.reserved}
            step = plan.bfs(pos, slots) if slots else None
            if step is not None:
                plan.reserved.add(step[2])
                self._walk(plan, actions, w, [step[2]], "to forest")
            elif not resources and w is order[0]:
                # nothing known to mine: one worker looks for more (fog hides the rest of the map)
                self._explore(world, plan, actions, w, "worker explores")

    def _scout(self, world, plan, actions, u, relays, enemy_at):
        pos = u["pos"]
        if not relays:
            # no relay known: find more of the map (and the enemy)
            if not any(n in neighbours(pos) for n in enemy_at) and self._explore(world, plan, actions, u, "scout explores"):
                return
            return self._fight(world, plan, actions, u, list(enemy_at.values()), enemy_at, None, pos, [])
        target = min(relays, key=lambda r: manhattan(r["pos"], pos))
        if manhattan(target["pos"], pos) == 1:
            plan.reserved.add(pos)
            self.counts["capture relay"] = self.counts.get("capture relay", 0) + 1
            return
        self._walk(plan, actions, u, neighbours(target["pos"]), "to relay")

    def _medic(self, world, plan, actions, u, army, rally):
        pos = u["pos"]
        hurt = [n for n in neighbours(pos) if world.at.get(n) and world.at[n]["player"] == self.pid
                and world.at[n]["name"] in UNITS and world.at[n]["vars"].get("health", 0) < world.at[n]["vars"].get("max_health", 0)]
        if hurt:
            self._emit(actions, "heal", pos, direction_to(pos, hurt[0]), "heal")
            return
        wounded = [a for a in army if a is not u and a["vars"].get("health", 0) < a["vars"].get("max_health", 0)]
        friends = wounded or [a for a in army if a is not u and a["name"] in FIGHTERS]
        if friends:
            f = min(friends, key=lambda a: manhattan(a["pos"], pos))
            if manhattan(f["pos"], pos) > 1:
                self._walk(plan, actions, u, neighbours(f["pos"]), "follow army")
        elif manhattan(pos, rally) > 2:
            self._walk(plan, actions, u, [rally] + neighbours(rally), "rally")
        else:
            self._make_room(world, plan, actions, u)

    def _fight(self, world, plan, actions, u, enemies, enemy_at, enemy_base, rally, threat, remembered=()):
        pos, kind = u["pos"], u["name"]
        if kind == "tank" and u["vars"].get("cooldown") == 1:
            adj = [n for n in neighbours(pos) if n in enemy_at]
            if adj:
                t = min(adj, key=lambda q: TARGET_PRIORITY[enemy_at[q]["name"]])
                self._emit(actions, "attack", pos, direction_to(pos, t), f"attack {DISPLAY[enemy_at[t]['name']]}")
            plan.reserved.add(pos)
            return
        if kind == "marine":
            shots = [(pos[0] + 2 * v[0], pos[1] + 2 * v[1], k) for k, v in DIRS.items()]
            shots = [(x, y, k) for x, y, k in shots if (x, y) in enemy_at]
            if shots:
                x, y, k = min(shots, key=lambda t: TARGET_PRIORITY[enemy_at[(t[0], t[1])]["name"]])
                self._emit(actions, "shoot", pos, k, f"shoot {DISPLAY[enemy_at[(x, y)]['name']]}")
                return
        adj = [n for n in neighbours(pos) if n in enemy_at]
        if adj:
            t = min(adj, key=lambda q: TARGET_PRIORITY[enemy_at[q]["name"]])
            self._emit(actions, "attack", pos, direction_to(pos, t), f"attack {DISPLAY[enemy_at[t]['name']]}")
            return
        close = [e for e in enemies if e["name"] in UNITS and manhattan(e["pos"], pos) <= 5]
        if threat and not self.offensive:
            close = close or threat
        if self.offensive or close:
            if close:
                tgt = min(close, key=lambda e: (manhattan(e["pos"], pos), TARGET_PRIORITY[e["name"]]))
                label = "engage"
            else:
                known = list(enemies) + list(remembered)
                towers = [e for e in known if e["name"] == "tower"]
                near_tower = [t for t in towers if manhattan(t["pos"], pos) <= 6]
                bases = [e for e in known if e["name"] == "base"]
                pool = near_tower or bases or known
                if not pool:
                    # nothing known: head for where the enemy base should be
                    if enemy_base is not None:
                        self._walk(plan, actions, u, [enemy_base] + neighbours(enemy_base), "search")
                    return
                tgt = min(pool, key=lambda e: manhattan(e["pos"], pos))
                label = "march"
            if kind == "marine":
                goals = [(tgt["pos"][0] + 2 * v[0], tgt["pos"][1] + 2 * v[1]) for v in DIRS.values()]
                goals = [g for g in goals if world.inside(g)]
                if self._walk(plan, actions, u, goals, label):
                    return
            self._walk(plan, actions, u, neighbours(tgt["pos"]), label)
        elif manhattan(pos, rally) > 2:
            self._walk(plan, actions, u, [rally] + neighbours(rally), "rally")
        else:
            self._make_room(world, plan, actions, u)

    def _make_room(self, world, plan, actions, u):
        """Idle units step off the cells next to friendly production buildings."""
        def crowds(c):
            return any(world.at.get(n) is not None and world.at[n]["player"] == self.pid
                       and world.at[n]["name"] in ("barracks", "base") for n in neighbours(c))
        pos = u["pos"]
        if crowds(pos):
            away = [n for n in neighbours(pos) if plan.free(n) and not crowds(n)]
            if away:
                self._walk(plan, actions, u, away, "make room")

    def _rally_point(self, world, base, want):
        """The open cell reachable from the base that is closest to `want` (computed once)."""
        if getattr(self, "_rally", None) and self._rally[0] == want:
            return self._rally[1]
        static = {"forest", "rock", "water", "minerals", "gems", "relay"}
        seen = {base}
        q = deque([base])
        best = (manhattan(base, want), base)
        while q and len(seen) < 6000:
            cur = q.popleft()
            for n in neighbours(cur):
                o = world.at.get(n)
                if n in seen or not world.inside(n) or (o is not None and o["name"] in static):
                    continue
                seen.add(n)
                q.append(n)
                if o is None and manhattan(n, want) < best[0]:
                    best = (manhattan(n, want), n)
        self._rally = (want, best[1])
        return best[1]

    # ---------------------------------------------------------------- captions
    def _camera_focus(self, base, workers, fighters, enemy_base):
        if self.offensive and fighters and enemy_base:
            lead = min(fighters, key=lambda u: manhattan(u["pos"], enemy_base))
            return lead["pos"]
        units = fighters or workers
        if not units:
            return base
        xs = [u["pos"][0] for u in units] + [base[0]]
        ys = [u["pos"][1] for u in units] + [base[1]]
        return round(sum(xs) / len(xs)), round(sum(ys) / len(ys))

    def _track_research(self, world):
        for tid, (var, label, _, _) in TECHS.items():
            state = world.tech(var, self.pid)
            if self._last_tech.get(var, 0) != state and state == 2:
                self.events.append((world.tick, f"{label.upper()} DONE"))
            self._last_tech[var] = state
        self.events = [e for e in self.events if world.tick - e[0] < 6]

    def _caption(self, world, phase, res, workers, army):
        parts = [e[1] for e in self.events]
        order = sorted(self.counts, key=lambda k: (not k.isupper(), -self.counts[k]))
        parts += [f"{k} x{self.counts[k]}" if self.counts[k] > 1 else k for k in order]
        counts = {}
        for u in army:
            counts[DISPLAY[u["name"]]] = counts.get(DISPLAY[u["name"]], 0) + 1
        army_txt = " ".join(f"{n} {k}" for k, n in sorted(counts.items())) or "none"
        relays = len([o for o in world.of("relay_owned", self.pid)])
        tech = " ".join(TECHS[t][1] + ("+" if world.tech(TECHS[t][0], self.pid) == 2 else "..."
                                       if world.tech(TECHS[t][0], self.pid) == 1 else "")
                        for t in self.strategy["research"])
        return [f"{self.strategy['name']} {phase}  res={res} relays={relays}",
                f"workers={len(workers)} army: {army_txt}",
                f"tech: {tech}",
                ", ".join(parts[:3]) if parts else "(waiting)"]


class Match:
    """Runs two RTSPolicy players against each other on a Griddly game, simultaneously."""

    def __init__(self, game, p1, p2, seed=0):
        self.game = game
        self.players = {1: p1, 2: p2}
        self.policies = {1: RTSPolicy(1, 2, seed), 2: RTSPolicy(2, 1, seed)}
        self.width, self.height = game.get_width(), game.get_height()
        self.terminated = False

    def world(self, pid):
        """What player `pid` knows: the engine's fog-filtered state for that player."""
        return WorldView(self.game.get_player_state(pid), self.width, self.height)

    def step(self):
        """One game tick on the engine's clock: each policy records its actions from its own player
        state (``player.queue``), then the engine runs every recorded action together and advances
        time once (``game.tick()``; a real-time loop would call ``game.advance_time(dt)`` instead)."""
        plans = {pid: self.policies[pid].act(self.world(pid)) for pid in (1, 2)}
        for pid in (1, 2):
            for name, x, y, d in plans[pid][0]:
                self.players[pid].queue(name, [x, y, d])
        result = self.game.tick()
        info = result["Info"]
        if result["Terminated"]:
            self.terminated = True
        return {pid: plans[pid][1] for pid in (1, 2)}, {pid: plans[pid][0] for pid in (1, 2)}, info


def run_match(game, p1, p2, level, seed=0, max_ticks=1500, verbose=False):
    game.load_level(level)
    game.reset()
    match = Match(game, p1, p2, seed)
    info = {}
    for t in range(max_ticks):
        captions, _, info = match.step()
        if verbose and t % 50 == 0:
            print(t, "|", " / ".join(captions[1]), "||", " / ".join(captions[2]))
        if match.terminated:
            break
    # the full state, for the summary only (no policy reads it)
    world = WorldView(game.get_state(), match.width, match.height)
    winner = info.get("PlayerResults", {})
    summary = {pid: {"workers": len(world.of("worker", pid)), "army": len([o for o in world.objects if o["player"] == pid and o["name"] in ARMY]),
                     "relays": len(world.of("relay_owned", pid)), "res": world.resources(pid)} for pid in (1, 2)}
    return world.tick, winner, summary


def _headless():
    import os
    import sys
    os.environ.setdefault("GRIDDLY_SDL_WINDOW", "1")
    from griddly import GriddlyLoader

    here = os.path.dirname(os.path.realpath(__file__))
    gdy = GriddlyLoader().load(os.path.join(here, "GriddlyRTS-FoW.yaml"))
    game = gdy.create_game("Vector")
    p1 = game.register_player("Player 1", "Vector")
    p2 = game.register_player("Player 2", "Vector")
    game.init(False)
    levels = [int(a) for a in sys.argv[1:]] or range(gdy.get_level_count())
    for level in levels:
        ticks, winner, summary = run_match(game, p1, p2, level, verbose=len(levels) == 1)
        print(f"level {level}: {ticks} ticks, result {winner or 'time limit'}, {summary}")


if __name__ == "__main__":
    _headless()
