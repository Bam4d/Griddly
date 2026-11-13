"""Helpers shared by the action remasters (zelda, butterflies, spider_nest).

  * GDY builders for wandering enemies (absolute or relative/rotating random walks) and spawners that
    hatch onto EMPTY_OR_GROUND, so every object type is covered (an enemy whose move hits an
    unlisted object would stop scheduling itself and freeze).
  * Level helpers: rooms() (grid of rooms joined by doorways), lock_path() (lock doorways on the
    route to a goal and drop one key per region), and check_keys() (exhaustive door-order search: the
    level must be solvable and no order of opening doors may dead-end).
"""
from __future__ import annotations

import collections

import common as C

GROUND = set(C.GROUND_NAMES)


def _exec(action, delay):
    return f"{{exec: {{Action: {action}, Delay: {delay}, Randomize: true}}}}"


def wander(action: str, mover: str, delay: int, all_objects: list[str], special: dict[str, str],
           relative: bool = False) -> str:
    """Random-walk action for `mover`.

    `special` maps destination object name -> raw `Src: ..., Dst: ...` behaviour YAML (2 lines, indented
    6) for objects with custom effects (eat, die, kill). Every other non-ground object blocks the move
    and the walk is rescheduled.
    """
    ex = _exec(action, delay)
    blocked = [o for o in all_objects if o not in GROUND and o not in special and not (relative and o == mover)]
    if relative:
        inputs = f'''    InputMapping:
      Inputs:
        1: {{Description: Rotate left, OrientationVector: [-1, 0]}}
        2: {{Description: Move forwards, OrientationVector: [0, -1], VectorToDest: [0, -1]}}
        3: {{Description: Rotate right, OrientationVector: [1, 0]}}
      Relative: true
      Internal: true
'''
        selfrule = f"      - Src: {{Object: {mover}, Commands: [{{rot: _dir}}, {ex}]}}\n        Dst: {{Object: {mover}}}\n"
    else:
        inputs = "    InputMapping:\n      Internal: true\n"
        selfrule = ""
    rules = "".join(special.values())
    return f'''  - Name: {action}
{inputs}    Behaviours:
{selfrule}      - Src: {{Object: {mover}, Commands: [{{mov: _dest}}, {ex}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {mover}, Commands: [{ex}]}}
        Dst: {{Object: [{", ".join(blocked)}]}}
{rules}'''


def hatch(action: str, spawner: str, child: str, delay: int, retry: int, all_objects: list[str],
          limit: int | None = None, consume: bool = False, idle: int = 0) -> str:
    """Spawner that hatches `child` into a random neighbouring empty/ground cell, then reschedules
    itself after `delay` ticks (after `retry` ticks when the chosen cell is occupied).

    `limit`: stop after that many hatches (object variable `hatched`). `consume`: the spawner is
    removed when it hatches (one child each). `idle`: extra "wait" inputs aimed at the spawner itself,
    so each try only hatches with probability 4 / (4 + idle) - this spreads hatch times out.
    """
    blocked = [o for o in all_objects if o not in GROUND]
    pre = f"\n          Preconditions: [{{lt: [src.hatched, {limit}]}}]" if limit else ""
    if consume:
        cmds = f"[{{spawn: {child}}}, {{remove: true}}]"
    elif limit:
        cmds = f"[{{spawn: {child}}}, {{incr: hatched}}, {_exec(action, delay)}]"
    else:
        cmds = f"[{{spawn: {child}}}, {_exec(action, delay)}]"
    dirs = [("West", -1, 0), ("North", 0, -1), ("East", 1, 0), ("South", 0, 1)]
    inputs = "".join(f"        {i + 1}: {{Description: {d}, VectorToDest: [{x}, {y}]}}\n" for i, (d, x, y) in enumerate(dirs))
    inputs += "".join(f"        {5 + i}: {{Description: Wait, VectorToDest: [0, 0]}}\n" for i in range(idle))
    return f'''  - Name: {action}
    InputMapping:
      Internal: true
      Inputs:
{inputs}    Behaviours:
      - Src:
          Object: {spawner}{pre}
          Commands: {cmds}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src:
          Object: {spawner}{pre}
          Commands: [{_exec(action, retry)}]
        Dst: {{Object: [{", ".join(blocked)}]}}
'''


# ----------------------------------------------------------------------------- level helpers
def rooms(m, xs, ys, wall="W", extra=0.3, door=1):
    """Partition the box spanned by xs/ys (wall-line coordinates) into rooms joined by doorways along a
    random spanning tree plus `extra` chance of loop doorways. Returns box[row][col] interiors."""
    for x in xs:
        m.rect(x, ys[0], x, ys[-1], wall)
    for y in ys:
        m.rect(xs[0], y, xs[-1], y, wall)
    nc, nr = len(xs) - 1, len(ys) - 1
    box = [[(xs[i] + 1, ys[j] + 1, xs[i + 1] - 1, ys[j + 1] - 1) for i in range(nc)] for j in range(nr)]

    def doorway(a, b):
        (i, j), (k, l) = a, b
        if j == l:
            x = xs[max(i, k)]
            y0, y1 = ys[j] + 1, ys[j + 1] - 1
            y = m.rng.randint(y0 + 1, max(y0 + 1, y1 - door))
            for d in range(door):
                m.g[min(y + d, y1)][x] = "."
        else:
            y = ys[max(j, l)]
            x0, x1 = xs[i] + 1, xs[i + 1] - 1
            x = m.rng.randint(x0 + 1, max(x0 + 1, x1 - door))
            for d in range(door):
                m.g[y][min(x + d, x1)] = "."

    cells = [(i, j) for j in range(nr) for i in range(nc)]
    start = cells[0]
    seen, stack, edges = {start}, [start], set()
    while stack:
        i, j = stack[-1]
        nb = [(i + di, j + dj) for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1))
              if 0 <= i + di < nc and 0 <= j + dj < nr and (i + di, j + dj) not in seen]
        if not nb:
            stack.pop()
            continue
        n = m.rng.choice(nb)
        doorway((i, j), n)
        edges.add(frozenset(((i, j), n)))
        seen.add(n)
        stack.append(n)
    for i, j in cells:
        for n in ((i + 1, j), (i, j + 1)):
            if n[0] < nc and n[1] < nr and frozenset(((i, j), n)) not in edges and m.rng.random() < extra:
                doorway((i, j), n)
    return box


def centre(b):
    return (b[0] + b[2]) // 2, (b[1] + b[3]) // 2


def grid_rows(m):
    return ["".join(r) for r in m.g]


def bfs_path(rows, start, goal, passable):
    prev = {start: None}
    q = collections.deque([start])
    while q:
        p = q.popleft()
        if p == goal:
            break
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (p[0] + dx, p[1] + dy)
            if n in prev or not (0 <= n[1] < len(rows) and 0 <= n[0] < len(rows[0])):
                continue
            if n == goal or passable(rows[n[1]][n[0]]):
                prev[n] = p
                q.append(n)
    if goal not in prev:
        return None
    path, p = [], goal
    while p is not None:
        path.append(p)
        p = prev[p]
    return path[::-1]


def lock_path(m, start, goal, n_doors, walk, wall_chars, key="k", door="D", min_key_dist=4):
    """Lock `n_doors` doorways on the shortest route start->goal, then put one key in each region
    (before door 0, between doors, and one for the goal itself in the last region)."""
    rows = grid_rows(m)
    path = bfs_path(rows, start, goal, lambda c: c in walk)
    assert path, "lock_path: goal unreachable"
    gaps = []
    for x, y in path[1:-1]:
        h = rows[y][x - 1] in wall_chars and rows[y][x + 1] in wall_chars
        v = rows[y - 1][x] in wall_chars and rows[y + 1][x] in wall_chars
        if h or v:
            gaps.append((x, y))
    assert len(gaps) >= n_doors, f"lock_path: only {len(gaps)} doorways on the route"
    idx = [round((i + 1) * len(gaps) / (n_doors + 1)) for i in range(n_doors)]
    doors = sorted({gaps[min(i, len(gaps) - 1)] for i in idx}, key=path.index)
    on_path = set(path)
    # make every door a real cut: wall off doorways on alternative routes around it
    for i, d in enumerate(doors):
        closed = set(doors[:i + 1])
        while True:
            rows = grid_rows(m)
            prev = {start: None}
            q = collections.deque([start])
            while q:
                p = q.popleft()
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    n = (p[0] + dx, p[1] + dy)
                    if n in prev or n in closed:
                        continue
                    if n == goal or rows[n[1]][n[0]] in walk:
                        prev[n] = p
                        q.append(n)
            if goal not in prev:
                break
            p, route = goal, []
            while p is not None:
                route.append(p)
                p = prev[p]
            cand = [(x, y) for x, y in route[1:-1] if (x, y) not in on_path and (
                (rows[y][x - 1] in wall_chars and rows[y][x + 1] in wall_chars) or
                (rows[y - 1][x] in wall_chars and rows[y + 1][x] in wall_chars))]
            assert cand, f"lock_path: cannot seal the route around door {d}"
            x, y = cand[len(cand) // 2]
            nb = [rows[y + dy][x + dx] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
            m.set(x, y, next(c for c in nb if c in wall_chars))
    for d in doors:
        m.set(*d, door)
    rows = grid_rows(m)
    prev_reach: set = set()
    for i in range(len(doors) + 1):
        closed = set(doors[i:])
        r = C.reach(rows, start, lambda c, p: (c in walk or c == door) and p not in closed and p != goal)
        region = [p for p in r - prev_reach if m.get(*p) == "." and p != start
                  and abs(p[0] - start[0]) + abs(p[1] - start[1]) >= min_key_dist]
        if not region:
            region = [p for p in r if m.get(*p) == "." and p != start]
        far = sorted(region, key=lambda p: -min(abs(p[0] - d[0]) + abs(p[1] - d[1]) for d in [start] + doors))
        p = m.rng.choice(far[:max(3, len(far) // 3)])
        m.set(*p, key)
        prev_reach = r
    return doors


def check_keys(rows, walk, key="k", door="D", goal="E", targets="", avatar="A"):
    """Exhaustive search over door-opening orders. Every key is assumed collected once reachable.
    Asserts the level can be won (goal adjacent with a key in hand / every target reachable) and that
    no reachable state is a dead end. Returns (#states, #doors)."""
    start = C.find(rows, avatar)[0]
    keys = C.find(rows, key)
    doors = C.find(rows, door)
    goals = C.find(rows, goal) if goal else []
    tgts = [p for t in targets for p in C.find(rows, t)]
    passable = set(walk) | {key, avatar} | set(targets)

    def reach(opened):
        return C.reach(rows, start, lambda c, p: c in passable or p in opened)

    adj = lambda p, s: any((p[0] + dx, p[1] + dy) in s for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    seen, stack, wins = set(), [frozenset()], 0
    while stack:
        opened = stack.pop()
        if opened in seen:
            continue
        seen.add(opened)
        r = reach(opened)
        avail = sum(k in r for k in keys) - len(opened)
        won = all(t in r for t in tgts) and (not goals or (avail >= 1 and any(adj(g, r) for g in goals)))
        if won:
            wins += 1
            continue
        nxt = [d for d in doors if d not in opened and adj(d, r)] if avail >= 1 else []
        assert nxt, f"dead end after opening {sorted(opened)} (keys left {avail})"
        stack.extend(opened | {d} for d in nxt)
    assert wins, "level cannot be won"
    return len(seen), len(doors)


def far_cells(m, start, n, reachable, min_dist, region=None):
    """n random free cells from `reachable`, at least min_dist (manhattan) from start."""
    spots = [p for p in m.free(region) if p in reachable and abs(p[0] - start[0]) + abs(p[1] - start[1]) >= min_dist]
    m.rng.shuffle(spots)
    return spots[:n]


# ----------------------------------------------------------------------------- hunters (A* chasers)
# A hunter is a spider that chases the avatar with `exec ... Search:` (A*). The avatar publishes its
# position in the globals ax / ay (SYNC after every successful move + an initial sync), and each hunter
# runs a self-scheduled `stalk` think action every `pace` ticks:
#   * d = |x - ax| + |y - ay|; it becomes alert when d <= sense and loses track when d > lose (or, with
#     `hide`, when the avatar stands in hiding (global `hidden` = 1) and is more than 2 tiles away);
#   * alert: one A* step towards the avatar (`hunt` with Search; ImpassableObjects = every solid object
#     incl. other spiders/hunters, so hunters queue up and flank); otherwise a random step;
#   * pace (ticks between steps) = `pace`, dropping to fast[2] once the global fast[0] >= fast[1].
# Everything is deterministic for a given seed (no Triggers), so recorded runs replay exactly.
# Engine notes: A* returns action 0 when the avatar is unreachable / gone; the exec then falls back to
# the stalk action's own [0, 0] vector, i.e. the hunter waits. stalk reschedules itself before it
# moves, so a hunter that burns up (lava / sword flash) simply drops its pending think.
SYNC = "{exec: {Action: sync, ActionId: 1}}"
SYNC_INIT = "      - Action: sync\n        ActionId: 1\n"
HUNTER_VARS = ["rx", "ry", "arx", "ary", "d", "pace", "alert", "due"]


def sync_action(avatar: str) -> str:
    return f'''  # Publishes the avatar's position in globals ax / ay (read by the hunters' stalk logic).
  - Name: sync
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Sync, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {avatar}, Commands: [{{set: [ax, _x]}}, {{set: [ay, _y]}}]}}
        Dst: {{Object: {avatar}}}
'''


def hunter_globals(extra: dict[str, int] | None = None) -> str:
    vals = {"ax": 0, "ay": 0, **(extra or {})}
    return "".join(f"    - Name: {k}\n      InitialValue: {v}\n" for k, v in vals.items())


def hunter_object_extra(first_delay: int = 1) -> str:
    """Variables + InitialActions for a hunter object (pass as obj(extra=...), tiles=2)."""
    v = "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: 0\n" for n in HUNTER_VARS)
    return v + f"    InitialActions:\n      - Action: stalk\n        ActionId: 1\n        Delay: {first_delay}\n"


def hunter(mover: str, avatar: str, all_objects: list[str], special: dict[str, str], sense: int = 9,
           lose: int = 14, pace: int = 3, fast: tuple[str, int, int] | None = None, hide: bool = False,
           passable: tuple[str, ...] = ()) -> str:
    """GDY for the `stalk` (think) and `hunt` (step) actions of a hunter object `mover`.

    `special` maps destination object -> raw behaviour YAML for the hunt step (kill the avatar, burn in
    lava, eat butterflies...); without a rule the step is simply blocked. `passable`: extra objects A*
    may path through (e.g. butterflies it eats); everything else non-ground (except the avatar, the
    target) is impassable for the search.
    """
    imp = [o for o in all_objects if o not in GROUND and o != avatar and o not in passable]
    search = f"Search: {{TargetObjectName: {avatar}, ImpassableObjects: [{', '.join(imp)}]}}"
    speed = (f"            - if: {{Conditions: {{gte: [{fast[0]}, {fast[1]}]}}, OnTrue: [{{set: [pace, {fast[2]}]}}]}}\n"
             if fast else "")
    hide_cmd = ("            - if: {Conditions: {and: [{eq: [hidden, 1]}, {gt: [d, 2]}]}, OnTrue: [{set: [alert, 0]}]}\n"
                if hide else "")
    rules = "".join(special.values())
    return f'''  # Hunter think: every `pace` ticks decide - chase the {avatar} (A*) when alert, else roam.
  - Name: stalk
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: {mover}
          Commands:
            - set: [rx, _x]
            - sub: [rx, ax]
            - set: [arx, rx]
            - if: {{Conditions: {{lt: [rx, 0]}}, OnTrue: [{{set: [arx, 0]}}, {{sub: [arx, rx]}}]}}
            - set: [ry, _y]
            - sub: [ry, ay]
            - set: [ary, ry]
            - if: {{Conditions: {{lt: [ry, 0]}}, OnTrue: [{{set: [ary, 0]}}, {{sub: [ary, ry]}}]}}
            - set: [d, arx]
            - add: [d, ary]
            - if: {{Conditions: {{lte: [d, {sense}]}}, OnTrue: [{{set: [alert, 1]}}]}}
            - if: {{Conditions: {{gt: [d, {lose}]}}, OnTrue: [{{set: [alert, 0]}}]}}
{hide_cmd}            - set: [pace, {pace}]
{speed}            - set: [due, _steps]
            - add: [due, pace]
            - exec: {{Action: stalk, ActionId: 1, Delay: pace}}
            - if:
                Conditions: {{eq: [alert, 1]}}
                OnTrue: [{{set_tile: 1}}, {{exec: {{Action: hunt, {search}}}}}]
                OnFalse: [{{set_tile: 0}}, {{exec: {{Action: hunt, Randomize: true}}}}]
        Dst: {{Object: {mover}}}

  # One hunter step (absolute 1 W, 2 N, 3 E, 4 S); chosen by A* or at random by stalk.
  - Name: hunt
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: {mover}, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
{rules}'''
