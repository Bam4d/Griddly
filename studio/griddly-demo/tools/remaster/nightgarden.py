#!/usr/bin/env python3
"""Night Garden: sneak through a hedge garden at night, as seen over your shoulder. Collect every lantern,
then slip out through the garden gate without being seen by the patrolling guards.

An EGOCENTRIC game: the thief turns and walks (1 turn left, 2 forward, 3 turn right, 4 back up
keeping your facing), and the default cameras rotate with them, so you only see what's ahead and around you.

Rules
  * Guards (`G`) patrol fixed routes along the gravel paths, deterministically: they step forward every
    2nd tick and turn on the route's corner stones (`r` turn right, `l` turn left, `u` turn around;
    a blocked guard also turns around). Their facing is set in the level (G[R] etc).
  * Every tick a guard looks up to 3 tiles straight ahead. Hedges, trees, bushes, water, walls,
    statues, the gate and other guards block the view. If it sees you, you're caught (lose). Walking into a
    guard, or being walked into, also gets you caught.
  * Tall grass (`T`) hides you completely while you stand in it.
  * Lanterns (`o`, +1) must all be collected. Then walk into the garden gate (`E`, +5) to escape (win).
    The global `need` counts the lanterns still missing (HUD).

GDY implementation: each guard runs a self-scheduled `patrol` action (alternating step/pause via the
`mv` variable), then `look1..look3` (relative internal actions at 1..3 tiles ahead) which stop at the first
blocker (`clear` flag) and catch an avatar whose `hidden` variable is 0. Corner stones are Z=0 objects
the guard steps onto, which `exec` a relative turn (`gturn`).

Layout characters (besides the shared terrain chars from common):
  A thief (A[U/R/D/L])   G guard (G[...])   o lantern   E gate   T tall grass   w garden wall
  r / l / u route corner stones (turn right / left / around)   . lawn
Route cells are drawn as gravel (sand). Validation: rectangular; lanterns and gate reachable; guard
routes closed and walkable; a 2000-tick replica simulation of the guards (same rules as the GDY) checks
that guards never collide or hit a blocker and that the start is never watched.
"""
from __future__ import annotations

import collections
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import ego as E  # noqa: E402

GID = "nightgarden"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "hedge", "stone", "tombstone",
                                                               "torch"))
SAND = G["sand"]
MARKERS = {"r": 1, "l": -1, "u": 2}
GROUNDISH = set(".") | set(G.values()) | set(MARKERS) | {"T"}
DV = {"U": (0, -1), "R": (1, 0), "D": (0, 1), "L": (-1, 0)}
DIRS4 = ((1, 0), (-1, 0), (0, 1), (0, -1))
RANGE = 3


def grid(m):
    return ["".join(r) for r in m.g]


def walkable(c):
    return c in GROUNDISH or c in "oAG"


def sdir(a, b):
    dx, dy = (b[0] > a[0]) - (b[0] < a[0]), (b[1] > a[1]) - (b[1] < a[1])
    return next(k for k, v in DV.items() if v == (dx, dy))


class Level:
    def __init__(self, w, h, seed, border=HEDGE):
        self.m = C.Map(w, h, seed=seed, border=border)
        self.guards = []      # (pos, facing)
        self.routes = []      # list of cell lists
        self.orient = {}

    def route(self, pts, start=0, loop=True, start_t=0.5):
        """A patrol route through the waypoints (axis-aligned segments). loop: closed loop (last
        waypoint connects back to the first); otherwise ping-pong between the ends. The guard starts on
        segment `start`, start_t of the way along it."""
        m = self.m
        segs = list(zip(pts, pts[1:] + ([pts[0]] if loop else [])))
        cells = []
        for a, b in segs:
            assert a[0] == b[0] or a[1] == b[1], f"diagonal segment {a}->{b}"
            d = DV[sdir(a, b)]
            p = a
            while p != b:
                cells.append(p)
                p = (p[0] + d[0], p[1] + d[1])
        if not loop:
            cells.append(pts[-1])
        for p in cells:
            assert m.get(*p) in ".T" + SAND, f"route cell {p} is {m.get(*p)!r}"
            m.set(*p, SAND)
        # corner stones
        if loop:
            for i, p in enumerate(pts):
                din = sdir(pts[i - 1], p)
                dout = sdir(p, pts[(i + 1) % len(pts)])
                m.set(*p, self._marker(din, dout))
        else:
            for i in range(1, len(pts) - 1):
                m.set(*pts[i], self._marker(sdir(pts[i - 1], pts[i]), sdir(pts[i], pts[i + 1])))
            m.set(*pts[0], "u")
            m.set(*pts[-1], "u")
        a, b = segs[start]
        L = abs(a[0] - b[0]) + abs(a[1] - b[1])
        k = max(1, min(L - 1, int(L * start_t)))
        d = DV[sdir(a, b)]
        pos = (a[0] + d[0] * k, a[1] + d[1] * k)
        self.guards.append((pos, sdir(a, b)))
        self.routes.append(cells)
        return pos

    @staticmethod
    def _marker(din, dout):
        order = "URDL"
        k = (order.index(dout) - order.index(din)) % 4
        return {0: SAND, 1: "r", 3: "l", 2: "u"}[k]

    def finish(self, start, facing, gate, lanterns, grass_spots, deco):
        m = self.m
        for p, ch in deco:
            if m.get(*p) == ".":
                m.set(*p, ch)
        for cx, cy, r in grass_spots:
            m.blob(cx, cy, r, "T", only=".")
        for (x, y) in lanterns:   # nearest lawn / tall-grass cell to the requested spot
            spots = [(abs(x - p[0]) + abs(y - p[1]), p) for p in m.free() + m.free(ch="T")]
            m.set(*min(spots)[1], "o")
        m.set(*gate, "E", margin=0)
        m.set(*start, "A")
        self.orient[start] = facing
        for pos, f in self.guards:
            m.set(*pos, "G")
            self.orient[pos] = f
        return self


# ----------------------------------------------------------------------------- guard replica
def simulate(rows, guards, ticks, start=None):
    """Replica of the GDY guard logic. guards: list of [x, y, facing, mv]. Yields watched cells per tick.
    Asserts guards never block on anything (routes must be explicit) and never collide."""
    H, W = len(rows), len(rows[0])
    order = "URDL"
    gs = [list(g) for g in guards]
    occ_static = lambda x, y: rows[y][x]
    for t in range(ticks):
        for g in gs:
            if g[3] == 1:
                g[3] = 0
                d = DV[g[2]]
                n = (g[0] + d[0], g[1] + d[1])
                c = occ_static(*n)
                others = {(o[0], o[1]) for o in gs if o is not g}
                assert c in GROUNDISH - {"T"} and n not in others, f"guard at {g[:2]} blocked by {c!r} at {n} (t={t})"
                g[0], g[1] = n
                if c in MARKERS:
                    g[2] = order[(order.index(g[2]) + MARKERS[c]) % 4]
            else:
                g[3] = 1
        watched = set()
        poss = {(o[0], o[1]) for o in gs}
        for g in gs:
            d = DV[g[2]]
            for r in range(1, RANGE + 1):
                x, y = g[0] + d[0] * r, g[1] + d[1] * r
                if not (0 <= x < W and 0 <= y < H):
                    break
                if (x, y) in poss or not (rows[y][x] in GROUNDISH or rows[y][x] in "oA"):
                    break
                watched.add((x, y))
        yield t, poss, watched


# ----------------------------------------------------------------------------- levels
def moonlit_lawn():
    L = Level(27, 15, seed=401)
    m = L.m
    m.blob(13, 7, 1.6, WATER)                                # the fountain
    for x, y in [(5, 3), (5, 11), (21, 3), (21, 11)]:
        m.rect(x - 1, y, x + 1, y, BUSH)                     # flower beds
    L.route([(9, 4), (17, 4), (17, 10), (9, 10)], start=0)   # loop around the fountain (clockwise)
    L.route([(23, 2), (23, 12)], loop=False, start=0)        # sentry pacing by the gate
    deco = [((2, 7), TREE), ((3, 2), TREE), ((3, 12), TREE), ((12, 1), TORCH), ((14, 13), TORCH),
            ((19, 7), TREE), ((7, 7), TREE)]
    L.finish((1, 7), "R", (26, 7), [(13, 3), (13, 12), (6, 7), (20, 12)],
             [(11, 2, 1.3), (15, 12, 1.2), (5, 8, 1.2), (20, 7, 1.0), (25, 4, 0.9), (25, 10, 0.9)], deco)
    return L, dict(grass=8, moss=4, mush=2)


def rose_maze():
    L = Level(29, 17, seed=402)
    m = L.m
    # garden rooms made of hedges
    m.rect(7, 1, 7, 11, HEDGE)
    m.rect(14, 5, 14, 11, HEDGE)
    m.rect(21, 1, 21, 11, HEDGE)
    m.rect(8, 11, 13, 11, HEDGE)
    m.rect(15, 5, 20, 5, HEDGE)
    for p in [(7, 9), (21, 3), (11, 11)]:                                 # doorways
        m.set(*p, ".")
    L.route([(9, 2), (19, 2)], loop=False, start=0, start_t=0.3)          # top corridor (behind hedges)
    L.route([(4, 13), (25, 13)], loop=False, start=0, start_t=0.6)        # long south walk
    L.route([(16, 7), (19, 7), (19, 10), (16, 10)], start=0)            # little loop in the east room
    deco = [((10, 7), TREE), ((11, 5), BUSH), ((12, 8), BUSH), ((3, 4), TREE), ((25, 5), TREE),
            ((24, 8), BUSH), ((10, 14), TORCH), ((18, 15), TORCH)]
    L.finish((2, 2), "D", (28, 8), [(12, 3), (18, 8), (5, 15), (26, 2), (24, 15)],
             [(4, 9, 1.4), (11, 9, 1.0), (18, 12, 0.9), (25, 10, 1.3), (16, 3, 0.9), (3, 15, 0.9), (10, 15, 0.9)],
             deco)
    return L, dict(grass=10, moss=3, mush=3)


def fountain_court():
    L = Level(31, 19, seed=403)
    m = L.m
    m.blob(15, 9, 1.8, WATER)
    for x0, y0 in [(5, 3), (21, 3), (5, 13), (21, 13)]:
        m.rect(x0, y0, x0 + 4, y0 + 2, BUSH)                               # flower beds
        m.rect(x0 + 1, y0 + 1, x0 + 3, y0 + 1, "T")                        # ... with tall grass inside
        m.set(x0 + 2, y0 + (2 if y0 < 9 else 0), "T")                      # an opening towards the court
    m.rect(14, 3, 16, 3, STONE)                                            # statues
    m.rect(14, 15, 16, 15, STONE)
    L.route([(11, 7), (19, 7), (19, 11), (11, 11)], start=0)               # inner loop (clockwise)
    L.route([(3, 1), (3, 17), (27, 17), (27, 1)], start=0, start_t=0.2)    # outer loop (anticlockwise)
    deco = [((1, 1), TREE), ((1, 17), TREE), ((29, 1), TREE), ((29, 17), TREE), ((12, 5), TORCH), ((18, 13), TORCH)]
    L.finish((1, 9), "R", (30, 9), [(15, 5), (7, 4), (23, 14), (15, 13), (28, 4)],
             [(9, 9, 1.1), (21, 9, 1.1), (12, 2, 0.8), (18, 16, 0.8)], deco)
    return L, dict(grass=10, moss=5, mush=2)


def orchard():
    L = Level(33, 19, seed=404)
    m = L.m
    for x in range(5, 29, 4):                                              # tree rows
        for y in range(2, 17):
            if y not in (5, 9, 13) and m.rng.random() < 0.85:
                m.set(x, y, TREE)
    for x in range(5, 29, 4):
        m.set(x, 9, ".")
    m.rect(11, 7, 15, 11, ".")                                             # two clearings
    m.rect(19, 7, 23, 11, ".")
    m.blob(13, 9, 0.6, WATER)                                              # a well in each
    m.blob(21, 9, 0.6, WATER)
    L.route([(2, 5), (30, 5)], loop=False, start=0, start_t=0.3)
    L.route([(2, 13), (30, 13)], loop=False, start=0, start_t=0.7)
    L.route([(11, 7), (15, 7), (15, 11), (11, 11)], start=0)
    L.route([(23, 7), (23, 11), (19, 11), (19, 7)], start=0)
    deco = [((1, 1), TORCH), ((31, 17), TORCH)]
    L.finish((1, 9), "R", (32, 9), [(7, 3), (17, 15), (25, 3), (29, 16), (9, 16), (13, 9)],
             [(3, 3, 1.0), (7, 15, 1.0), (10, 2, 0.8), (18, 2, 0.9), (26, 15, 0.9), (30, 9, 0.8), (21, 15, 0.8),
              (14, 3, 0.8), (6, 9, 0.6), (26, 9, 0.6), (17, 9, 0.6)], deco)
    return L, dict(grass=12, moss=3, mush=6)


def manor_gardens():
    L = Level(35, 21, seed=405)
    m = L.m
    m.rect(13, 7, 21, 13, STONE)                                           # the manor house
    m.rect(14, 8, 20, 12, "w")
    for x0, y0 in [(3, 3), (27, 3), (3, 15), (27, 15)]:
        m.rect(x0, y0, x0 + 4, y0 + 2, HEDGE, filled=False)                # hedge gardens with tall grass
        m.rect(x0 + 1, y0 + 1, x0 + 3, y0 + 1, "T")
        m.set(x0 + 2, y0 + (2 if y0 < 10 else 0), "T")
    m.blob(6, 10, 1.3, WATER)
    m.blob(28, 10, 1.3, WATER)
    L.route([(11, 5), (23, 5), (23, 15), (11, 15)], start=0)               # around the manor
    L.route([(1, 1), (33, 1)], loop=False, start=0, start_t=0.5)           # north wall walk
    L.route([(1, 19), (33, 19)], loop=False, start=0, start_t=0.2)         # south wall walk
    L.route([(9, 8), (9, 12)], loop=False, start=0)                        # west sentry
    L.route([(25, 12), (25, 8)], loop=False, start=0)                      # east sentry
    deco = [((12, 3), TREE), ((22, 3), TREE), ((12, 17), TREE), ((22, 17), TREE), ((17, 4), TORCH), ((17, 16), TORCH),
            ((3, 10), TREE), ((31, 10), TREE)]
    L.finish((1, 10), "R", (34, 10), [(5, 4), (29, 4), (5, 16), (29, 16), (17, 3), (17, 17), (31, 7)],
             [(15, 3, 0.8), (19, 17, 0.8), (7, 7, 0.8), (27, 13, 0.8), (31, 13, 0.7), (3, 7, 0.7)], deco)
    return L, dict(grass=10, moss=6, mush=3)


LEVELS = [
    ("Moonlit Lawn", "One guard circles the fountain, another paces by the gate.", moonlit_lawn),
    ("Rose Maze", "Hedge rooms and long gravel walks. Hide in the tall grass.", rose_maze),
    ("Fountain Court", "Two guards circle in opposite directions. Time your dash.", fountain_court),
    ("Moonlit Orchard", "Rows of apple trees and four guards on their rounds.", orchard),
    ("The Manor Gardens", "Seven lanterns all around the manor, and its whole night watch.", manor_gardens),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows, orient):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one thief"
    a = a[0]
    gate = C.find(rows, "E")
    assert len(gate) == 1, f"{name}: need exactly one gate"
    los = C.find(rows, "o")
    r = C.reach(rows, a, lambda c, p: walkable(c) and c != "G")
    assert all(p in r for p in los), f"{name}: unreachable lantern"
    gx, gy = gate[0]
    assert any((gx + dx, gy + dy) in r for dx, dy in DIRS4), f"{name}: gate unreachable"
    guards = [[x, y, orient[(x, y)], 0] for (x, y) in C.find(rows, "G")]
    static = [r_.replace("G", ".") for r_ in rows]   # guards are tracked by the simulation
    static = [row.replace("G", ".") for row in static]
    watched_ever = set()
    for t, poss, watched in simulate(static, guards, 2000):
        watched_ever |= watched
        assert a not in watched or rows[a[1]][a[0]] == "T", f"{name}: start watched at t={t}"
        assert a not in poss, f"{name}: guard walks over the start"
    hidden_lanterns = sum(rows[y][x] == "T" for x, y in los)
    return len(los), len(guards), len(watched_ever), hidden_lanterns


# ----------------------------------------------------------------------------- GDY
BLOCKERS = ["wall", "gate", "lantern", "guard", "water", "tree", "bush", "hedge", "stone", "tombstone", "torch"]
SIGHT_BLOCKERS = ["wall", "gate", "guard", "water", "tree", "bush", "hedge", "stone", "tombstone", "torch"]
TERRAIN = set(C.GROUND_NAMES) | {"water", "tree", "bush", "hedge", "stone", "torch", "tombstone"}
MARKER_OBJ = {"r": "turn_right", "l": "turn_left", "u": "turn_back"}


def look(k):
    return f'''  - Name: look{k}
    InputMapping:
      Internal: true
      Relative: true
      Inputs:
        1: {{Description: Look {k} ahead, VectorToDest: [0, -{k}]}}
    Behaviours:
      - Src: {{Object: guard, Commands: [{{set: [clear, 0]}}]}}
        Dst: {{Object: [{", ".join(SIGHT_BLOCKERS)}]}}
      - Src: {{Object: guard, Preconditions: [{{eq: [dst.hidden, 0]}}], Commands: [{{set: [clear, 0]}}]}}
        Dst: {{Object: avatar, Commands: [{{remove: true}}]}}
'''


def actions():
    walk = C.EMPTY_OR_GROUND[:-1] + ", turn_right, turn_left, turn_back]"
    return f'''
Actions:
{E.wait_action("avatar")}
  # Relative movement: 1 turn left, 2 forward, 3 turn right, 4 back up (facing kept).
  - Name: move
{E.REL_MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{rot: _dir}}]}}
        Dst: {{Object: avatar}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{set: [hidden, 0]}}]}}
        Dst: {{Object: {walk}}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{set: [hidden, 1]}}]}}
        Dst: {{Object: tall_grass}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{set: [hidden, 0]}}, {{decr: need}}, {{reward: 1}}]}}
        Dst: {{Object: lantern, Commands: [{{remove: true}}]}}
      - Src:
          Object: avatar
          Preconditions: [{{eq: [meta.fwd, 1]}}, {{eq: [need, 0]}}]
          Commands: [{{set: [escaped, 1]}}, {{reward: 5}}]
        Dst: {{Object: gate}}
      - Src: {{Object: avatar, Commands: [{{remove: true}}]}}
        Dst: {{Object: guard}}

  # Every lantern adds itself to `need` when the level starts.
  - Name: count_lantern
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: lantern, Commands: [{{incr: need}}]}}
        Dst: {{Object: lantern}}

  # Guard brain, every tick: step forward every 2nd tick, then look up to 3 tiles ahead.
  - Name: patrol
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Patrol, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: guard
          Commands:
            - if: {{Conditions: {{eq: [mv, 1]}}, OnTrue: [{{set: [mv, 0]}}, {{exec: {{Action: gstep, ActionId: 1}}}}], OnFalse: [{{set: [mv, 1]}}]}}
            - set: [clear, 1]
            - exec: {{Action: look1, ActionId: 1}}
            - if: {{Conditions: {{eq: [clear, 1]}}, OnTrue: [{{exec: {{Action: look2, ActionId: 1}}}}]}}
            - if: {{Conditions: {{eq: [clear, 1]}}, OnTrue: [{{exec: {{Action: look3, ActionId: 1}}}}]}}
            - exec: {{Action: patrol, ActionId: 1, Delay: 1}}
        Dst: {{Object: guard}}

  # One step along the route; corner stones turn the guard, anything in the way turns it around.
  - Name: gstep
    InputMapping:
      Internal: true
      Relative: true
      Inputs:
        1: {{Description: Step forward, VectorToDest: [0, -1]}}
    Behaviours:
      - Src: {{Object: guard, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND[:-1] + ", tall_grass]"}}}
      - Src: {{Object: guard, Commands: [{{mov: _dest}}, {{exec: {{Action: gturn, ActionId: 3}}}}]}}
        Dst: {{Object: turn_right}}
      - Src: {{Object: guard, Commands: [{{mov: _dest}}, {{exec: {{Action: gturn, ActionId: 1}}}}]}}
        Dst: {{Object: turn_left}}
      - Src: {{Object: guard, Commands: [{{mov: _dest}}, {{exec: {{Action: gturn, ActionId: 2}}}}]}}
        Dst: {{Object: turn_back}}
      - Src: {{Object: guard, Commands: [{{nop: []}}]}}
        Dst: {{Object: avatar, Commands: [{{remove: true}}]}}
      - Src: {{Object: guard, Commands: [{{exec: {{Action: gturn, ActionId: 2}}}}]}}
        Dst: {{Object: [{", ".join(BLOCKERS)}]}}

  - Name: gturn
    InputMapping:
      Internal: true
      Relative: true
      Inputs:
        1: {{Description: Turn left, OrientationVector: [-1, 0]}}
        2: {{Description: Turn around, OrientationVector: [0, 1]}}
        3: {{Description: Turn right, OrientationVector: [1, 0]}}
    Behaviours:
      - Src: {{Object: guard, Commands: [{{rot: _dir}}]}}
        Dst: {{Object: guard}}

{look(1)}{look(2)}{look(3)}'''


def objects():
    gravel = C.img(C.TG + "floor_sand_a.png")
    v = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: 0\n" for n in names)
    init = lambda act, delay=0: (f"    InitialActions:\n      - Action: {act}\n        ActionId: 1\n"
                                 + (f"        Delay: {delay}\n" if delay else ""))
    return "\nObjects:\n" + C.terrain_objects(TERRAIN) + "".join([
        C.obj("wall", "w", 1, C.WALL_STYLES["stone"], "[0.5, 0.5, 0.5]", "square", 1.0),
        C.obj("tall_grass", "T", 0, C.img("gvgai/oryx/bush4.png"), "[0.1, 0.6, 0.4]", "square", 1.0),
        C.obj("turn_right", "r", 0, gravel, "[0.6, 0.5, 0.3]", "square", 0.9),
        C.obj("turn_left", "l", 0, gravel, "[0.6, 0.5, 0.3]", "square", 0.9),
        C.obj("turn_back", "u", 0, gravel, "[0.6, 0.5, 0.3]", "square", 0.9),
        C.obj("gate", "E", 1, C.img("gvgai/oryx/doorclosed1.png"), "[0.8, 0.6, 0.1]", "square", 1.0),
        C.obj("lantern", "o", 1, C.img("gvgai/oryx/candle1.png"), "[1.0, 0.9, 0.3]", "circle", 0.6, init("count_lantern")),
        C.obj("guard", "G", 2, C.img("oryx/oryx_fantasy/avatars/knight1.png"), "[0.9, 0.1, 0.1]", "triangle", 0.9,
              v(["mv", "clear"]) + init("patrol", 1)),
        C.obj("avatar", "A", 2, C.img("gvgai/oryx/rogue.png"), "[0.3, 0.9, 0.9]", "triangle", 0.8, v(["hidden"])),
    ])


def main():
    hdr = E.header("Night Garden", """
        Sneak through a hedge garden at night, seen over your shoulder (egocentric, rotating view).
        Collect every lantern and slip out through the gate. Guards patrol fixed routes and see 3 tiles
        straight ahead; tall grass hides you.
        """.replace("        ", ""), "avatar", ["eq: [escaped, 1]"], ["eq: [avatar:count, 0]"],
        background="gvgai/oryx/floor3.png",
        variables="    - Name: need\n      InitialValue: 0\n    - Name: escaped\n      InitialValue: 0\n")
    parts = [hdr]
    for i, (name, blurb, fn) in enumerate(LEVELS):
        L, recipe = fn()
        rows = grid(L.m)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
        nl, ng, nw, nh = validate(name, rows, L.orient)
        ground = C.decorate(rows, recipe, seed=8400 + i, stack_under="AoG")
        # gravel under guards (they always start on a route)
        for (x, y), _ in L.guards:
            ground[y][x] = SAND
        # objects that are Z=0 can't have ground under them; the gravel route is already its own tile
        for y, row in enumerate(rows):
            for x, c in enumerate(row):
                if c in "Trlu" + SAND:
                    ground[y][x] = ""
        parts.append(E.level_block(name, blurb, rows, ground, L.orient))
        print(f"{i}. {name:18s} {len(rows[0])}x{len(rows)} lanterns={nl} guards={ng} watched-cells={nw}",
              file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Night Garden",
        "tagline": "Steal the lanterns. Stay out of the guards' sight.",
        "description": "A stealth game seen over your shoulder in a first-person, rotating view. Guards walk "
                       "fixed rounds through the hedge garden and see three tiles straight ahead. Hide in "
                       "tall grass, time your moves, collect every lantern and slip out through the gate.",
        "tags": ["stealth", "egocentric", "partial-observability", "timing"],
        "difficulty": "medium",
        "original": "",
        "accent": "#818cf8",
        "icon": "🏮",
        "controls": {"move": "move", "scheme": "relative",
                     "extra": [{"key": " ", "label": "Wait", "action": "wait"}]},
        "hud": [
            {"label": "Lanterns left", "kind": "global", "variable": "need", "tone": "amber"},
            {"label": "Hidden", "kind": "var", "object": "avatar", "variable": "hidden", "tone": "emerald"},
            {"label": "Guards", "kind": "count", "object": "guard", "tone": "rose"},
        ],
        "win_text": "You slip out through the gate with every lantern. Nobody saw a thing.",
        "lose_text": "Caught by the night watch!",
    })


if __name__ == "__main__":
    main()
