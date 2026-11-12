#!/usr/bin/env python3
"""Bait Remastered: fill the pits with boxes to reach the key, then unlock the door.

Remaster of resources/games/Single-Player/GVGAI/bait.yaml. Same rules: a box pushed into a pit fills
it (box and pit both vanish); walking into a pit is fatal; stepping on the key picks it up
(avatar variable has_key); bumping the door while holding the key opens it and wins; toadstools are
optional bonus pickups (+1). (bait_keys showed the key on the avatar sprite via set_tile; the avatar's
render tiles are now its 4 facing sprites, so the demo HUD shows has_key instead.)

Levels are themed room-and-corridor layouts (pushlib). The key (and in later levels the door) sits in
a part of the map cut off by a band of pits; boxes are scattered in the open part. Every candidate is
solved forward with a search over (avatar, boxes, pits, has_key) states and the solvable one needing
the most pushes is kept. Toadstools are modelled as solid for boxes (they are: no rule lets a box
onto one), so the solver's solution stays valid whether or not they have been eaten. Solutions go to
apps/web/src/games/solutions/bait.json for replay_solutions.mjs.

Layout characters (besides the shared terrain chars from common; digits are avoided because the level
parser reads digits as player ids):
  w wall   A avatar   b box   h pit   k key   g door (goal)   m toadstool   . floor
"""
from __future__ import annotations

import collections
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import pushlib as P  # noqa: E402

GID = "bait"

LEVELS = [
    dict(name="Cellar Door", blurb="A pit guards the key. Fill it, grab the key, open the door.", theme="warehouse",
         seed=301, cols=3, rows=2, w=26, h=15, key_cut=2, door_cut=0, spare=1, mush=2),
    dict(name="Garden Moat", blurb="Hedges, trees and a moat of pits around the key.", theme="garden", seed=312,
         cols=3, rows=2, w=28, h=16, key_cut=3, door_cut=0, spare=2, mush=3),
    dict(name="Thin Ice", blurb="Holes in the ice everywhere. Look before you step.", theme="frozen", seed=323,
         cols=3, rows=2, w=28, h=16, key_cut=2, door_cut=2, spare=1, mush=3),
    dict(name="Sealed Crypt", blurb="Both the key and the crypt door lie beyond open graves.", theme="crypt",
         seed=334, cols=3, rows=2, w=30, h=19, key_cut=3, door_cut=2, spare=1, mush=4),
    dict(name="Mine Shaft", blurb="Bridge the shafts of the old quarry.", theme="quarry", seed=345,
         cols=4, rows=2, w=32, h=17, key_cut=3, door_cut=3, spare=2, mush=4, wide=0.25, tries=10),
]


class BaitRules:
    """State = (avatar, frozenset(boxes), frozenset(pits), has_key). Static: floor (walkable cells incl.
    key/door/toadstool cells), key, door, toadstools (solid for boxes)."""

    def __init__(self, floor, key, door, mush):
        self.floor, self.key, self.door, self.mush = set(floor), key, door, set(mush)
        self.adj = P.make_adj(self.floor - {door})

    def start(self, player, boxes, pits):
        return (player, frozenset(boxes), frozenset(pits), False)

    def blockers(self, s):
        out = s[1] | s[2]
        return out if s[3] else out | {self.key}

    def passable(self, s, n):
        return n in self.floor and n != self.door and n not in self.blockers(s)

    def events(self, s, par):
        out = [(P.add(b, (-d[0], -d[1])), m) for b in s[1] for m, d in P.DIRS.items()
               if P.add(b, (-d[0], -d[1])) in par]
        for target in ((self.door,) if s[3] else (self.key,)):
            out += [(P.add(target, (-d[0], -d[1])), m) for m, d in P.DIRS.items() if P.add(target, (-d[0], -d[1])) in par]
        return out

    def step(self, s, cell, m):
        d = P.DIRS[m]
        n = P.add(cell, d)
        boxes, pits, has_key = s[1], s[2], s[3]
        if not has_key and n == self.key:
            return (n, boxes, pits, True), False
        if has_key and n == self.door:
            return s, True
        if n not in boxes:
            return None
        t = P.add(n, d)
        if t in pits:
            return (n, boxes - {n}, pits - {t}, has_key), False
        if t not in self.floor or t in boxes or t == self.door or t in self.mush or (t == self.key and not has_key):
            return None
        # a box in a corner (walls/door/toadstools) can never move again: only fine if no pits need it
        solid = lambda p: p not in self.floor or p == self.door or p in self.mush
        if (solid(P.add(t, (1, 0))) or solid(P.add(t, (-1, 0)))) and (solid(P.add(t, (0, 1))) or solid(P.add(t, (0, -1)))):
            if len(boxes) - 1 < len(pits):
                return None
        return (n, frozenset(boxes - {n} | {t}), pits, has_key), False

    def done(self, s):
        return False

    def h(self, s):
        return len(s[2]) * 2 + (0 if s[3] else 1)


def bfs_dist(cells, src):
    dist = {src: 0}
    q = collections.deque([src])
    while q:
        c = q.popleft()
        for d in P.DIRS.values():
            n = P.add(c, d)
            if n in cells and n not in dist:
                dist[n] = dist[c] + 1
                q.append(n)
    return dist


def cut_around(cells, target, start, width, max_size=3):
    """A band of pits `width` deep around `target` that separates it from `start`: cells at BFS
    distance r+1..r+width from target, for the r with the smallest band (<= max_size per layer)."""
    dt = bfs_dist(cells, target)
    best = None
    for r in range(2, dt[start] - width - 2):
        band = [c for c in cells if r < dt[c] <= r + width]
        layer = [c for c in band if dt[c] == r + 1]
        if len(layer) > max_size:
            continue
        rest = cells - set(band)
        if start in bfs_dist(rest, target):
            continue
        if best is None or len(band) < len(best) or (len(band) == len(best) and r > best_r):
            best, best_r = band, r
    return best


def build(spec):
    m, _, recipe = P.themed_layout(spec["w"], spec["h"], spec["seed"], spec["theme"], spec["cols"], spec["rows"],
                                   rw=(6, 8), rh=(4, 6), wide=spec.get("wide", 0.0), props_scale=0.6)
    fill = P.THEMES[spec["theme"]][0]
    base = ["".join(r) for r in P.crop(m, fill + "w")]
    floor = {(x, y) for y, r in enumerate(base) for x, c in enumerate(r) if c == "."}
    best = None
    fails = collections.Counter()
    for i in range(spec.get("attempts", 30)):
        if fails["unsolved"] + (best is not None) * 2 >= spec.get("tries", 4):
            break
        rng = P.attempt_rng(spec["seed"], i)
        cells = sorted(floor)
        rng.shuffle(cells)
        start = cells[0]
        ds = bfs_dist(floor, start)
        far = sorted(floor, key=lambda c: -ds[c])
        key = rng.choice(far[:max(3, len(far) // 8)])
        pits = cut_around(floor, key, start, spec["key_cut"] and 1 + (spec["key_cut"] > 2), max_size=2)
        if not pits:
            fails["keycut"] += 1
            continue
        pits = set(pits)
        open_ = set(bfs_dist(floor - pits, start))
        door = None
        if spec["door_cut"]:
            dd = [c for c in open_ if ds[c] > 8 and sum(P.add(c, d) in floor for d in P.DIRS.values()) >= 2]
            rng.shuffle(dd)
            for door in dd:
                band = cut_around(open_, door, start, 1, max_size=2)
                if band:
                    pits |= set(band)
                    break
            else:
                fails["doorcut"] += 1
                continue
        else:
            # door against a wall of the open part, far-ish from the start
            dd = [c for c in open_ if ds[c] > 6 and sum(P.add(c, d) in floor for d in P.DIRS.values()) == 3]
            if not dd:
                fails["door"] += 1
                continue
            door = rng.choice(dd)
        open_ = set(bfs_dist(floor - pits - {door}, start))
        # boxes on open interior cells, pushable in both axes
        nbox = len(pits) + spec["spare"]
        roomy = [c for c in open_ if c != start and all(P.add(c, d) in open_ for d in P.DIRS.values())]
        rng.shuffle(roomy)
        boxes = []
        for c in roomy:
            if all(abs(c[0] - b[0]) + abs(c[1] - b[1]) >= 3 for b in boxes):
                boxes.append(c)
            if len(boxes) == nbox:
                break
        if len(boxes) < nbox:
            fails["boxes"] += 1
            continue
        taken = set(boxes) | pits | {key, door, start}
        mush = [c for c in cells if c not in taken and sum(P.add(c, d) in floor for d in P.DIRS.values()) == 2
                and all(abs(c[0] - q[0]) + abs(c[1] - q[1]) > 1 for q in taken)][:spec["mush"]]
        rules = BaitRules(floor, key, door, mush)
        st = rules.start(start, boxes, pits)
        sol = P.solve(st, rules, max_nodes=spec.get("max_nodes", 12_000), weight=spec.get("weight", 2.0))
        if not sol:
            fails["unsolved"] += 1
            continue
        moves, events = sol
        assert P.replay(st, rules, moves), f"{spec['name']}: replay mismatch"
        score = events + len(pits) * 5
        if best is None or score > best[0]:
            best = (score, moves, events, start, boxes, pits, key, door, mush)
    assert best, f"{spec['name']}: no solvable placement {dict(fails)}"
    _, moves, events, start, boxes, pits, key, door, mush = best
    g = [list(r) for r in base]
    for ch, ps in (("h", pits), ("b", boxes), ("m", mush), ("k", [key]), ("g", [door]), ("A", [start])):
        for x, y in ps:
            g[y][x] = ch
    rows = ["".join(r) for r in g]
    ground = C.decorate(rows, recipe, seed=spec["seed"] + 5000, stack_under="Abhkgm")
    return rows, ground, moves, events, len(pits)


def validate(name, rows):
    C.check_rect(name, rows)
    for ch in "Akg":
        assert len(C.find(rows, ch)) == 1, f"{name}: need exactly one {ch}"
    assert len(C.find(rows, "b")) >= len(C.find(rows, "h")) > 0, f"{name}: boxes/pits"


SCENERY = ", ".join(C.SCENERY_NAMES)

ACTIONS = f'''
Actions:
{C.face_action("avatar")}
  # Absolute 4-way movement. Walking into a box pushes it; a box pushed into a pit fills it (both
  # vanish). Walking into a pit is fatal. Step on the key to pick it up; bump the door holding the key
  # to win. Toadstools are bonus points. Boxes only slide over floor and ground decoration.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: box, Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, {SCENERY}]}}
      - Src: {{Object: box, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: box, Commands: [{{remove: true}}, {{reward: 1}}]}}
        Dst: {{Object: pit, Commands: [{{remove: true}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: pit}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}, {{mov: _dest}}]}}
        Dst: {{Object: toadstool, Commands: [{{remove: true}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{incr: has_key}}]}}
        Dst: {{Object: key, Commands: [{{remove: true}}]}}
      - Src:
          Object: avatar
          Preconditions: [{{eq: [src.has_key, 1]}}]
          Commands: [{{set_tile: meta.facing}}, {{reward: 5}}]
        Dst: {{Object: goal, Commands: [{{remove: true}}]}}
      - Src:
          Object: avatar
          Preconditions: [{{eq: [src.has_key, 0]}}]
          Commands: [{{set_tile: meta.facing}}]
        Dst: {{Object: goal}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/oryx/swordman1_0.png", "bait-avatar")
    av_extra = "    Variables:\n      - Name: has_key\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "w", 1, C.tiled("gvgai/oryx/dirtWall", "_"), "[0.5, 0.5, 0.5]", "square", 0.9),
        C.obj("pit", "h", 1, C.img("gvgai/newset/hole1.png"), "[0.4, 0.4, 0.4]", "square", 0.7),
        C.obj("key", "k", 1, C.img("gvgai/oryx/key2.png"), "[0.8, 0.8, 0.2]", "triangle", 0.5),
        C.obj("goal", "g", 1, C.img("gvgai/oryx/doorclosed1.png"), "[0.0, 0.2, 1.0]", "square", 0.8),
        C.obj("toadstool", "m", 1, C.img("gvgai/oryx/mushroom1.png"), "[0.0, 0.8, 0.2]", "square", 0.5),
        C.obj("box", "b", 2, C.img("gvgai/newset/block3.png"), "[0.2, 0.6, 0.2]", "square", 0.8),
        C.obj("avatar", "A", 2, C.imgs(avatar), "[0.0, 1.0, 0.0]", "triangle", 0.8, av_extra, tiles=4),
    ])


def main():
    parts = [C.header("Bait Remastered", """
        Get the key and unlock the door. Pits block the way: push boxes into them to fill them in, and
        never step into one yourself. Toadstools are a tasty bonus. Walls, water, hedges, trees, rocks
        and tombstones block boxes; grass, sand, moss and dirt don't.
        """.replace("        ", ""), "avatar", ["eq: [goal:count, 0]"], ["eq: [avatar:count, 0]"],
        background="gvgai/oryx/backLBrown.png")]
    sols = []
    for i, spec in enumerate(LEVELS):
        t = time.time()
        rows, ground, moves, events, npits = build(spec)
        validate(spec["name"], rows)
        parts.append(C.level_block(spec["name"], spec["blurb"], rows, ground))
        sols.append(moves)
        print(f"{i}. {spec['name']:13s} {len(rows[0])}x{len(rows)} pits={npits} boxes={len(C.find(rows, 'b'))} "
              f"solution: {events} events, {len(moves)} moves ({time.time() - t:.0f}s)", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    P.write_solutions(GID, sols)
    gdy = "".join(parts) + ACTIONS + objects()
    C.emit(GID, gdy, {
        "name": "Bait Remastered",
        "tagline": "Fill the pits, grab the key, open the door.",
        "description": "Push boxes into the pits to build a way to the key, then unlock the door. Cellars, "
                       "garden moats, thin ice, a sealed crypt and an old mine shaft. Don't fall in!",
        "tags": ["puzzle", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/bait.yaml",
        "accent": "#d97706",
        "icon": "🗝️",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Key", "kind": "var", "object": "avatar", "variable": "has_key", "tone": "amber"},
                {"label": "Pits left", "kind": "remaining", "object": "pit", "tone": "rose"},
                {"label": "Toadstools", "kind": "collected", "object": "toadstool", "tone": "emerald"}],
        "win_text": "The door is open!",
        "lose_text": "You fell into a pit.",
    })


if __name__ == "__main__":
    main()
