"""Push-puzzle solver helpers shared by the remastered push games (sokoban, sokoban2, clusters, bait).

A game supplies a `Rules` object:
  * passable(state, cell)  -> the avatar can simply walk onto `cell` (no event happens)
  * step(state, cell, d)   -> the avatar stands on `cell` and moves in direction d onto a cell that is
                              not passable: returns (new_state, won) or None when nothing useful
                              happens (blocked / the avatar or a box would be destroyed)
  * done(state)            -> optional win test on states (e.g. no boxes left)
  * h(state)               -> heuristic (0 = plain BFS / uniform cost)
States are tuples whose first element is the exact avatar position. Visited states are keyed by the
avatar's reachable region (its smallest cell) + the rest of the state, so the search runs over
"events" (pushes, pickups) while the walking in between is reconstructed with BFS paths.

`solve` returns the move list (1 left, 2 up, 3 right, 4 down: common.MOVE_INPUTS) plus the number of
events, or None. Move ids match the engine's `move` action, so the list can be replayed verbatim.
"""
from __future__ import annotations

import collections
import heapq
import json
from pathlib import Path

DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
SOLUTIONS_DIR = Path(__file__).resolve().parent / "solutions"  # read by replay_solutions.mjs


def add(p, d):
    return p[0] + d[0], p[1] + d[1]


def region(state, rules):
    """BFS over cells the avatar can walk to. Returns parent map {cell: (prev, move)}.
    Fast path: rules.adj {cell: [(move, next)]} over static walkable cells + rules.blockers(state)."""
    start = state[0]
    adj = getattr(rules, "adj", None)
    if adj is not None:
        blk = rules.blockers(state)
        par = {start: None}
        q = [start]
        for c in q:
            for m, n in adj[c]:
                if n not in par and n not in blk:
                    par[n] = (c, m)
                    q.append(n)
        return par
    par = {start: None}
    q = collections.deque([start])
    while q:
        c = q.popleft()
        for m, d in DIRS.items():
            n = add(c, d)
            if n not in par and rules.passable(state, n):
                par[n] = (c, m)
                q.append(n)
    return par


def make_adj(cells):
    return {c: [(m, add(c, d)) for m, d in DIRS.items() if add(c, d) in cells] for c in cells}


def path_to(par, cell):
    out = []
    while par[cell] is not None:
        cell, m = par[cell]
        out.append(m)
    return out[::-1]


def solve(start, rules, max_nodes=400_000, weight=1.0):
    """Best-first (A* with `weight` on h; weight 0 h == BFS over events). Returns (moves, events) or None."""
    def key(state, par):
        return (min(par),) + tuple(state[1:])

    par0 = region(start, rules)
    k0 = key(start, par0)
    best = {k0: 0}
    links = {k0: None}  # key -> (prev_key, prev_state, moves)
    counter = 0
    heap = [(weight * rules.h(start), 0, counter, start, k0, par0)]
    nodes = 0
    while heap:
        _, g, _, state, k, par = heapq.heappop(heap)
        if best.get(k, 1e18) < g:
            continue
        nodes += 1
        if nodes > max_nodes:
            return None
        cands = rules.events(state, par) if hasattr(rules, "events") else \
            [(cell, m) for cell in par for m, d in DIRS.items() if add(cell, d) not in par
             and not rules.passable(state, add(cell, d))]
        for cell, m in cands:
            if True:
                res = rules.step(state, cell, m)
                if res is None:
                    continue
                ns, won = res
                if won or rules.done(ns):
                    seq = [path_to(par, cell) + [m]]
                    kk = k
                    while links[kk] is not None:
                        pk, mv = links[kk]
                        seq.append(mv)
                        kk = pk
                    allm = [x for part in reversed(seq) for x in part]
                    return allm, g + 1
                npar = region(ns, rules)
                nk = key(ns, npar)
                if nk in best and best[nk] <= g + 1:
                    continue
                best[nk] = g + 1
                links[nk] = (k, path_to(par, cell) + [m])
                counter += 1
                heapq.heappush(heap, (g + 1 + weight * rules.h(ns), g + 1, counter, ns, nk, npar))
    return None


def replay(start, rules, moves):
    """Re-simulate a move list with the rules (sanity check of solve's bookkeeping). Returns won."""
    state = start
    for i, m in enumerate(moves):
        n = add(state[0], DIRS[m])
        if rules.passable(state, n):
            state = (n,) + tuple(state[1:])
            continue
        res = rules.step(state, state[0], m)
        assert res is not None, f"replay: move {i} ({m}) has no effect"
        state, won = res
        if won or rules.done(state):
            return i == len(moves) - 1
    return False


def box_distances(cells, goals):
    """Push distance from every cell to the nearest goal cell, for a box alone on `cells` (reverse
    pulls: a box at c can be pushed to c+d when c-d is also a cell). Cells not in the result are dead."""
    dist = {g: 0 for g in goals}
    q = collections.deque(goals)
    while q:
        c = q.popleft()
        for d in DIRS.values():
            prev = add(c, (-d[0], -d[1]))       # box came from prev (pushed in direction d)
            behind = add(prev, (-d[0], -d[1]))  # avatar stood here
            if prev in cells and behind in cells and prev not in dist:
                dist[prev] = dist[c] + 1
                q.append(prev)
    return dist


def write_solutions(game_id, sols):
    SOLUTIONS_DIR.mkdir(parents=True, exist_ok=True)
    (SOLUTIONS_DIR / f"{game_id}.json").write_text(json.dumps(sols))


class Grid:
    """Static level grid: rows of chars; helpers to find chars."""

    def __init__(self, rows):
        self.rows = rows
        self.h, self.w = len(rows), len(rows[0])

    def at(self, p):
        x, y = p
        if 0 <= y < self.h and 0 <= x < self.w:
            return self.rows[y][x]
        return "w"

    def cells(self, chars):
        return {(x, y) for y, r in enumerate(self.rows) for x, c in enumerate(r) if c in chars}


# ----------------------------------------------------------------------------------------------
# Shared push-puzzle rules helpers
# ----------------------------------------------------------------------------------------------
import sys  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

FLOORISH = set(".A") | set(C.GROUND_CHARS.values())


def floor_cells(rows, extra=""):
    ok = FLOORISH | set(extra)
    return {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in ok}


def frozen_square(boxes, walls_or_static, b):
    """Simple 2x2 freeze deadlock: box b plus 3 neighbours in a 2x2 that are all boxes/static."""
    x, y = b
    for dx, dy in ((0, 0), (-1, 0), (0, -1), (-1, -1)):
        sq = [(x + dx, y + dy), (x + dx + 1, y + dy), (x + dx, y + dy + 1), (x + dx + 1, y + dy + 1)]
        if all(p in boxes or p in walls_or_static for p in sq):
            return True
    return False


def attempt_rng(seed, i):
    import random
    return random.Random(seed * 7919 + i)


def connect4(m, chars, fill):
    """Map.walk may step diagonally; open one orthogonal corner wherever two `chars` cells touch
    only diagonally."""
    for y in range(1, m.h - 2):
        for x in range(1, m.w - 2):
            for dx in (1, -1):
                if not 0 <= x + dx < m.w:
                    continue
                a, b = m.get(x, y), m.get(x + dx, y + 1)
                if a in chars and b in chars and m.get(x + dx, y) not in chars and m.get(x, y + 1) not in chars:
                    m.set(x + dx, y, fill)


def largest_region(rows, passable_chars):
    cells = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in passable_chars}
    best = set()
    seen = set()
    for c in cells:
        if c in seen:
            continue
        reg = C.reach(rows, c, lambda ch, p: ch in passable_chars)
        seen |= reg
        if len(reg) > len(best):
            best = reg
    return best


def fill_unreachable(m, keep, fill):
    """Turn floor not in `keep` into `fill` (so there are no sealed pockets)."""
    for y in range(m.h):
        for x in range(m.w):
            if m.g[y][x] == "." and (x, y) not in keep:
                m.g[y][x] = fill


# ----------------------------------------------------------------------------------------------
# Reverse-play ("pulling") generator for boxes-onto-goals puzzles
# ----------------------------------------------------------------------------------------------
def free_region(floor, boxes, p):
    seen, q = {p}, collections.deque([p])
    while q:
        c = q.popleft()
        for d in DIRS.values():
            n = add(c, d)
            if n in floor and n not in boxes and n not in seen:
                seen.add(n)
                q.append(n)
    return seen


def scramble(floor, goals, start, pulls, rng):
    """Start with a box on every goal and the player at `start`, then make `pulls` random reverse
    moves (the player pulls a box one step towards itself). Returns (boxes, player)."""
    boxes = set(goals)
    p = start
    if p in boxes:
        return None
    dist = box_distances(floor, list(goals))
    for _ in range(pulls):
        reg = free_region(floor, boxes, p)
        opts = []
        for c in reg:
            for d in DIRS.values():
                b = add(c, d)                    # box next to the player
                back = add(c, (-d[0], -d[1]))   # player steps back here, dragging the box to c
                if b in boxes and back in floor and back not in boxes:
                    gain = dist.get(c, 0) - dist.get(b, 0)
                    opts.append((gain, c, b, back))
        if not opts:
            break
        # prefer pulls that move boxes away from the goals
        opts.sort(key=lambda o: -o[0] + rng.random() * 2.5)
        _, c, b, back = opts[0] if rng.random() < 0.8 else rng.choice(opts)
        boxes.remove(b)
        boxes.add(c)
        p = back
    reg = sorted(free_region(floor, boxes, p) - set(goals))
    if not reg:
        return None
    return boxes, rng.choice(reg)


def pick_goals(floor, n, rng, region=None, min_free=3, spread=3):
    cand = [c for c in floor if (region is None or region(c))
            and sum(add(c, d) in floor for d in DIRS.values()) >= min_free]
    rng.shuffle(cand)
    out = []
    for c in cand:
        if all(abs(c[0] - o[0]) + abs(c[1] - o[1]) >= spread for o in out):
            out.append(c)
        if len(out) == n:
            return out
    return None


def best_goal_puzzle(name, floor, start, n_boxes, make_rules, seed, attempts=24, pulls=60, region=None,
                     max_nodes=60_000, weight=1.5, min_events=0, spread=3):
    """Try `attempts` scrambles; forward-solve each; keep the one with the most box pushes.
    make_rules(boxes, goals) -> (rules, start_state). Returns dict(boxes, goals, player, moves, events)."""
    best = None
    for i in range(attempts):
        rng = attempt_rng(seed, i)
        goals = pick_goals(floor, n_boxes, rng, region, spread=spread)
        if not goals:
            continue
        sc = scramble(floor, goals, start if start not in goals else rng.choice(sorted(floor - set(goals))),
                      pulls, rng)
        if not sc:
            continue
        boxes, player = sc
        if any(b in goals for b in boxes):
            continue
        rules, st = make_rules(boxes, goals, player)
        sol = solve(st, rules, max_nodes=max_nodes, weight=weight)
        if not sol:
            continue
        moves, events = sol
        assert replay(st, rules, moves), f"{name}: replay mismatch"
        score = events * 4 + len(moves) * 0.05
        if events >= min_events and (best is None or score > best["score"]):
            best = dict(boxes=boxes, goals=goals, player=player, moves=moves, events=events, score=score)
    assert best, f"{name}: no solvable scramble found"
    return best


# ----------------------------------------------------------------------------------------------
# Room-and-corridor layouts for push puzzles
# ----------------------------------------------------------------------------------------------
def rooms_layout(w, h, seed, fill, cols, rows_, rw=(5, 7), rh=(3, 5), loops=0.3, wide=0.0):
    """Fill the map with `fill`, carve cols x rows_ rooms (random sizes inside equal slots) joined by
    straight L-shaped 1-wide corridors along a random spanning tree (+ `loops` chance of extra links).
    Returns (Map, [room rects (x0, y0, x1, y1)])."""
    m = C.Map(w, h, seed, fill=fill, border=fill)
    sw, sh = (w - 2) // cols, (h - 2) // rows_
    rooms = []
    for j in range(rows_):
        for i in range(cols):
            W = min(m.rng.randint(*rw), sw - 2)
            H = min(m.rng.randint(*rh), sh - 2)
            x0 = 1 + i * sw + m.rng.randint(1, max(1, sw - W - 1))
            y0 = 1 + j * sh + m.rng.randint(1, max(1, sh - H - 1))
            r = (x0, y0, x0 + W - 1, y0 + H - 1)
            m.rect(*r, ".")
            rooms.append(r)
    cen = lambda r: ((r[0] + r[2]) // 2 + m.rng.randint(-1, 1), (r[1] + r[3]) // 2 + m.rng.randint(-1, 1))

    def corridor(a, b):
        (x, y), (tx, ty) = cen(rooms[a]), cen(rooms[b])
        horizontal_first = m.rng.random() < 0.5
        pts = [(x, y), (tx, y), (tx, ty)] if horizontal_first else [(x, y), (x, ty), (tx, ty)]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            for xx in range(min(x0, x1), max(x0, x1) + 1):
                for yy in range(min(y0, y1), max(y0, y1) + 1):
                    m.set(xx, yy, ".")
                    if wide and m.rng.random() < wide:
                        m.set(xx + (y0 != y1), yy + (x0 != x1), ".")

    idx = list(range(len(rooms)))
    nb = lambda k: [k2 for k2 in idx if abs(k2 % cols - k % cols) + abs(k2 // cols - k // cols) == 1]
    seen, stack, edges = {0}, [0], set()
    while stack:
        k = stack[-1]
        opts = [n for n in nb(k) if n not in seen]
        if not opts:
            stack.pop()
            continue
        n = m.rng.choice(opts)
        corridor(k, n)
        edges.add(frozenset((k, n)))
        seen.add(n)
        stack.append(n)
    for k in idx:
        for n in nb(k):
            if n > k and frozenset((k, n)) not in edges and m.rng.random() < loops:
                corridor(k, n)
    return m, rooms


def safe_props(m, ch, n, region=None, keep_open=None, wall_side=False, solid=""):
    """Place up to n solid props on floor without disconnecting the floor; never on `keep_open` cells
    and never where it would leave a floor cell with < 2 floor neighbours (keeps rooms pushable)."""
    rows = lambda: ["".join(r) for r in m.g]
    floor = lambda: {(x, y) for y in range(m.h) for x in range(m.w) if m.g[y][x] == "."}
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    for x, y in spots:
        if placed >= n:
            break
        if keep_open and (x, y) in keep_open:
            continue
        if wall_side and not any(m.g[y + dy][x + dx] in solid for dx, dy in DIRS.values()):
            continue
        f = floor()
        m.g[y][x] = ch
        f2 = f - {(x, y)}
        start = next(iter(f2))
        ok = len(C.reach(rows(), start, lambda c, p: c == ".")) == len(f2)
        if ok:
            ok = all(sum(add(q, d) in f2 for d in DIRS.values()) >= 2 for q in f2
                     if abs(q[0] - x) + abs(q[1] - y) == 1)
        if ok:
            placed += 1
        else:
            m.g[y][x] = "."
    return placed


def solid_dressing(m, fill, ch, prob, only_inner=True):
    """Replace some `fill` cells (not touching floor if only_inner) with scenery `ch` for looks."""
    for y in range(1, m.h - 1):
        for x in range(1, m.w - 1):
            if m.g[y][x] != fill:
                continue
            touches = any(m.g[y + dy][x + dx] == "." for dx in (-1, 0, 1) for dy in (-1, 0, 1))
            if (not only_inner or not touches) and m.rng.random() < prob:
                m.g[y][x] = ch


class SokoRules:
    """Rules of sokoban (vanish=True: a box pushed into a hole disappears together with the hole) and
    sokoban2 (vanish=False: boxes rest on targets and may be pushed off again).
    State = (player, frozenset(boxes), frozenset(open holes))."""

    def __init__(self, floor, goals, vanish):
        self.floor, self.vanish = set(floor), vanish
        self.adj = make_adj(self.floor)
        self.goals = frozenset(goals)
        self.dist = box_distances(self.floor, list(goals))

    def start(self, player, boxes):
        return (player, frozenset(boxes), frozenset(self.goals))

    def passable(self, s, n):
        return n in self.floor and n not in s[1]

    def blockers(self, s):
        return s[1]

    def events(self, s, par):
        out = []
        for b in s[1]:
            for m, d in DIRS.items():
                c = (b[0] - d[0], b[1] - d[1])
                if c in par:
                    out.append((c, m))
        return out

    def step(self, s, cell, m):
        d = DIRS[m]
        n = add(cell, d)
        boxes, holes = s[1], s[2]
        if n not in boxes:
            return None
        t = add(n, d)
        if t in boxes or t not in self.floor:
            return None
        if self.vanish and t in holes:
            return (n, boxes - {n}, holes - {t}), False
        if t not in self.dist:
            return None
        nb = (boxes - {n}) | {t}
        if self.frozen(nb, t):
            return None
        return (n, frozenset(nb), holes), False

    def frozen(self, boxes, b):
        x, y = b
        for dx, dy in ((0, 0), (-1, 0), (0, -1), (-1, -1)):
            sq = [(x + dx, y + dy), (x + dx + 1, y + dy), (x + dx, y + dy + 1), (x + dx + 1, y + dy + 1)]
            if all(p in boxes or p not in self.floor for p in sq):
                if any(p in boxes and (self.vanish or p not in self.goals) for p in sq):
                    return True
        return False

    def done(self, s):
        return not s[1] if self.vanish else s[1] <= self.goals

    def h(self, s):
        return sum(self.dist.get(b, 99) for b in s[1] if self.vanish or b not in self.goals)


# ----------------------------------------------------------------------------------------------
# Themes: structural fill, props inside rooms, scenery dressing of the solid mass, ground recipe
# ----------------------------------------------------------------------------------------------
S, G = C.SCENERY_CHARS, C.GROUND_CHARS
THEMES = {
    # name: (fill, [(prop char, count, wall_side)], [(dressing char, prob)], ground recipe)
    "warehouse": ("w", [(S["torch"], 6, True), (S["stone"], 3, False)], [], dict(dirt=5, sand=3, bones=2)),
    "garden": (S["hedge"], [(S["bush"], 6, False), (S["tree"], 3, False)], [(S["tree"], 0.25), (S["bush"], 0.1)],
               dict(grass=14, moss=3, mush=4)),
    "frozen": (S["water"], [(S["water"], 5, False), (S["rock"], 2, False), (S["tree"], 2, False)], [(S["rock"], 0.05)],
               dict(sand=8, moss=2, skull=2)),
    "crypt": (S["stone"], [(S["tombstone"], 6, False), (S["torch"], 6, True)], [],
              dict(dirt=6, moss=4, bones=8, skull=4)),
    "quarry": (S["rock"], [(S["rock"], 4, False), (S["torch"], 3, True)], [(S["water"], 0.04)],
               dict(dirt=10, sand=4, bones=3)),
    "orchard": (S["tree"], [(S["tree"], 5, False), (S["bush"], 4, False)], [(S["bush"], 0.15)],
                dict(grass=16, dirt=3, mush=6)),
}


def themed_layout(w, h, seed, theme, cols, rows_, rw=(5, 7), rh=(3, 5), loops=0.3, wide=0.0, props_scale=1.0):
    fill, props, dressing, recipe = THEMES[theme]
    m, rooms = rooms_layout(w, h, seed, fill, cols, rows_, rw, rh, loops, wide)
    for ch, n, wall_side in props:
        safe_props(m, ch, int(round(n * props_scale)), wall_side=wall_side, solid=fill + "w")
    for ch, p in dressing:
        solid_dressing(m, fill, ch, p)
    return m, rooms, recipe


def floor_of(m):
    return {(x, y) for y in range(m.h) for x in range(m.w) if m.g[y][x] == "."}


def crop(m, solid):
    rows = m.rows(solid=solid)
    return [list(r) for r in rows]


def build_goal_level(spec, vanish, box_ch="b", goal_ch="h"):
    """Layout + reverse-scramble + forward solve for sokoban / sokoban2. Returns (rows, ground, info)."""
    theme = spec["theme"]
    m, _, recipe = themed_layout(spec["w"], spec["h"], spec["seed"], theme, spec["cols"], spec["rows"],
                                 spec.get("rw", (5, 7)), spec.get("rh", (3, 5)), spec.get("loops", 0.3),
                                 spec.get("wide", 0.0), spec.get("props", 1.0))
    fill = THEMES[theme][0]
    g = crop(m, fill + "w")
    rows = ["".join(r) for r in g]
    floor = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == "."}
    region = largest_region(rows, ".")
    assert len(region) == len(floor), f"{spec['name']}: disconnected floor"
    start = min(floor)

    def make(boxes, goals, player):
        r = SokoRules(floor, goals, vanish)
        return r, r.start(player, boxes)

    best = best_goal_puzzle(spec["name"], floor, start, spec["boxes"], make, spec["seed"],
                            attempts=spec.get("attempts", 8), pulls=spec.get("pulls", 60),
                            max_nodes=spec.get("max_nodes", 20_000), weight=spec.get("weight", 1.5),
                            spread=spec.get("spread", 3))
    for x, y in best["goals"]:
        g[y][x] = goal_ch
    for x, y in best["boxes"]:
        g[y][x] = box_ch
    px, py = best["player"]
    g[py][px] = "A"
    rows = ["".join(r) for r in g]
    ground = C.decorate(rows, recipe, seed=spec["seed"] + 5000, stack_under="A" + box_ch + goal_ch)
    return rows, ground, best
