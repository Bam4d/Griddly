#!/usr/bin/env python3
"""Bufo's GPU Heist: a top-down stealth game in a Mistral datacenter (new "meme" game, no original).

Bufo sneaks between the server racks stealing GPUs and carries them back to the bufo burrow.

Rules (the GDY below; `simulate()` mirrors the patrol logic exactly for the planner)
  * Bufo moves with absolute 4-way `move` (1 left, 2 up, 3 right, 4 down) and can `wait` (Space).
  * Walking into a blinking server rack (`G`) steals its GPU (the rack goes dark). Bufo carries at most
    2 GPUs (`carry`, drawn as bufo hugging a GPU). Walking into the burrow (`V`) delivers everything
    carried. Win: every GPU in the level delivered (global `need` reaches 0).
  * Sysadmin cats (`C`, grey with headsets) patrol fixed routes marked with floor tape. They step every
    2nd tick; tape chevrons (u/r/d/l) turn them. Every tick a cat looks 3 tiles straight ahead (walls,
    racks, props and the burrow block its view). If it sees bufo, bufo is caught (lose). Bumping into a
    cat, or a cat walking into bufo, also gets bufo caught.
  * Security cats (`H`, `J`, `K`: brown with caps) patrol the same way but see 4 tiles. When one sees
    bufo it raises the alarm on itself: for 12 ticks (refreshed on every sighting) it chases bufo with A*
    (`exec` + `Search`) one step per tick, catching bufo on contact. Then it walks back (A*, every 2nd
    tick) to its cat bed (`p`, `q`, `o`), the first corner of its route, and resumes patrolling.
  * Cable bundles (`c`) are hiding spots: bufo standing in one is invisible, cats never walk on them and
    a chasing security cat cannot step in (A* treats them as impassable, and the catch needs the target
    to be out in the open). Hide and wait for the alarm to time out.
  Everything is deterministic (no randomness): every level ships a recorded winning run.

GDY implementation notes
  * Cats start with an orientation in the level string (C[R] etc.); `cat_init` turns it into the `dir`
    variable (1..4, = the absolute action ids) and starts the self-rescheduling `patrol` (Delay 1).
    Movement and sight use `exec` with `ActionId: dir`. Render tile = facing (up, right, down, left);
    security cats use tiles 4..7 (with a red "!") while chasing.
  * look1..look4 are internal absolute actions at 1..4 tiles; a `clear` flag stops sight at a blocker.
  * Security cats: `mode` 0 patrol, 1 chase (`alert` countdown), 2 return home. Each one has its own
    object name (hcat1..3) and bed object (bed1..3) so its "return home" A* can target its own bed.

Layout characters
  w wall  A bufo  V bufo burrow (delivery)  G rack with a GPU  R server rack (no GPU)  c cable bundle
  C sysadmin cat  H/J/K security cats 1..3  (cat beds p/q/o are placed by the generator)
  k coolant tank  P potted plant  M spinning Mistral logo sign  m Mistral logo floor decal  v floor vent
  t patrol tape   u r d l tape chevrons (turn up / right / down / left)   . raised floor

Validation: rectangular, one bufo, one burrow; routes are closed/ping-pong, walk only on free floor and
never collide; a space-time planner (BFS over (x, y, t) against the simulated patrols, never letting any
cat see bufo) finds a winning run for every level, written to apps/web/src/games/solutions/gpuheist.json.
`--check` replays those runs in the WASM engine (replay_solutions.mjs): the runs only win if the
engine's patrols match the simulation tick for tick.
"""
from __future__ import annotations

import collections
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

GID = "gpuheist"
HERE = Path(__file__).resolve().parent
SOLUTIONS = HERE.parents[1] / "apps/web/src/games/solutions" / f"{GID}.json"
P = M.PALETTE

CAP = 2          # GPUs bufo can carry
CAT_SIGHT = 3
HUNT_SIGHT = 4
ALERT = 12       # ticks a security cat keeps chasing after its last sighting
DV = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}        # action ids: left up right down
TILE = {1: 3, 2: 0, 3: 1, 4: 2}                             # render tile (up, right, down, left)
LETTER = {"L": 1, "U": 2, "R": 3, "D": 4}
MARK = {"l": 1, "u": 2, "r": 3, "d": 4}
HUNTERS = {"H": 1, "J": 2, "K": 3}
BEDS = {1: "p", 2: "q", 3: "o"}
BED_CH = {v: k for k, v in BEDS.items()}
ROUTE = set("t") | set(MARK) | set(BEDS.values())
FLOORISH = set(".mv") | ROUTE
SOLID = set("wGRVkPMZ")      # Z = emptied rack (only appears during planning)


# ----------------------------------------------------------------------------- art (original, memekit)
def _bang(px, x=18, y=1):
    M.rect(px, x, y, x + 4, y + 7, P["white"])
    M.rect(px, x + 1, y + 1, x + 3, y + 6, P["led_red"])
    M.rect(px, x + 2, y + 1, x + 2, y + 3, P["white"])
    M.put(px, x + 2, y + 5, P["white"])
    return px


def alert_cat():
    looks = {"up": (0, -1), "right": (1, 0), "down": (0, 1), "left": (-1, 0)}
    return [M.save(f"gpuheist-hcat-alert-{d}", M.add_arrow(_bang(M._cat_body("guard", looks[d])), d))
            for d in M.FACINGS]


def burrow():
    px = M.blank()
    M.ellipse(px, 12, 15, 11, 7.5, (150, 104, 60, 255), (186, 140, 90, 255), (104, 70, 40, 255))
    M.ellipse(px, 12, 15.5, 7, 4.2, (40, 26, 20, 255))
    M.ellipse(px, 12, 16.5, 5, 2.6, (18, 12, 10, 255))
    for x, y in ((9, 14), (15, 14)):          # a pair of eyes in the dark
        M.put(px, x, y, P["bufo_hi"])
    M.line(px, 19, 2, 19, 10, P["steel"])      # orange flag
    for i, k in enumerate(("mistral_yellow", "mistral_orange", "mistral_red")):
        M.rect(px, 20, 2 + i, 23 - i, 2 + i, P[k])
    M.outline(px)
    return M.save("gpuheist-burrow", px)


def cat_bed():
    px = M.blank()
    M.ellipse(px, 12, 14, 10, 6.5, (180, 60, 40, 255), (220, 100, 70, 255), (120, 36, 26, 255))
    M.ellipse(px, 12, 14.5, 6.5, 3.6, (250, 214, 160, 255))
    for x in (9, 12, 15):
        M.put(px, x, 14, (220, 170, 120, 255))
    M.outline(px)
    return M.save("gpuheist-catbed", px)


def tape(direction=None):
    """Floor tape (a dashed hazard square) with an optional chevron pointing the patrol's way."""
    px = M.blank()
    for i in range(3, 21):
        if (i // 2) % 2 == 0:
            for x, y in ((i, 3), (i, 20), (3, i), (20, i)):
                M.put(px, x, y, (240, 196, 40, 170))
    if direction:
        a = M.blank()
        M.tri(a, (12, 6), (6.5, 13), (17.5, 13), (250, 204, 21, 230))
        M.tri(a, (12, 10.5), (8.5, 15), (15.5, 15), M.T)
        M.rect(a, 10, 13, 13, 17, M.T)
        rot = {"u": 0, "r": 1, "d": 2, "l": 3}[direction]
        for _ in range(rot):
            a = [[a[23 - x][y] for x in range(24)] for y in range(24)]
        M.paste(px, a)
    return M.save(f"gpuheist-tape{'-' + direction if direction else ''}", px)


def vent():
    px = M.blank(P["floor_dark"])
    M.rect(px, 0, 0, 23, 0, P["floor"])
    M.rect(px, 0, 0, 0, 23, P["floor"])
    for y in range(3, 22, 3):
        M.rect(px, 3, y, 20, y, (60, 64, 76, 255))
        M.rect(px, 3, y + 1, 20, y + 1, (96, 102, 116, 255))
    return M.save("gpuheist-vent", px)


def plant():
    px = M.blank()
    for cx, cy, r in ((12, 8, 5), (7, 10, 4), (17, 10, 4), (10, 5, 3), (15, 5, 3)):
        M.ellipse(px, cx, cy, r, r * 0.9, (60, 150, 70, 255), (110, 200, 100, 255), (34, 96, 46, 255))
    M.rect(px, 7, 14, 16, 21, (200, 110, 60, 255))
    M.rect(px, 6, 13, 17, 14, (230, 140, 80, 255))
    M.rect(px, 15, 15, 16, 21, (160, 80, 40, 255))
    M.outline(px)
    return M.save("gpuheist-plant", px)


# ----------------------------------------------------------------------------- levels
class Level:
    def __init__(self, w, h, seed):
        self.m = C.Map(w, h, seed=seed, border="w")
        self.cats = []        # dict(pos, dir, kind: 'C' or hunter id)
        self.routes = []

    def put(self, cells, ch):
        for p in cells:
            self.m.set(*p, ch, margin=0)

    def row(self, y, x0, x1, ch="R"):
        self.m.rect(x0, y, x1, y, ch)

    def col(self, x, y0, y1, ch="R"):
        self.m.rect(x, y0, x, y1, ch)

    def route(self, pts, loop=True, start=0, start_t=0.5, kind="C"):
        """Patrol route through axis-aligned waypoints; loop or ping-pong. The cat starts on segment
        `start`, start_t along it. kind 'C' = sysadmin cat, 'H'/'J'/'K' = security cat (its bed goes on
        the first waypoint)."""
        m = self.m
        segs = list(zip(pts, pts[1:] + ([pts[0]] if loop else [])))
        cells = []
        for a, b in segs:
            assert a[0] == b[0] or a[1] == b[1], f"diagonal segment {a}->{b}"
            d = _dir(a, b)
            p = a
            while p != b:
                cells.append(p)
                p = (p[0] + DV[d][0], p[1] + DV[d][1])
        if not loop:
            cells.append(pts[-1])
        for p in cells:
            assert m.get(*p) in ".t", f"route cell {p} is {m.get(*p)!r}"
            m.set(*p, "t")
        marks = {}
        if loop:
            for i, p in enumerate(pts):
                marks[p] = _dir(p, pts[(i + 1) % len(pts)])
        else:
            for i in range(len(pts) - 1):
                marks[pts[i]] = _dir(pts[i], pts[i + 1])
            marks[pts[-1]] = _dir(pts[-1], pts[-2])
        inv = {v: k for k, v in MARK.items()}
        for p, d in marks.items():
            m.set(*p, inv[d])
        if kind in HUNTERS:   # the bed doubles as the route's first chevron: always "go right"
            assert marks[pts[0]] == 3, f"security route must leave its bed (first waypoint) going right"
            m.set(*pts[0], BEDS[HUNTERS[kind]])
        a, b = segs[start]
        L = abs(a[0] - b[0]) + abs(a[1] - b[1])
        k = max(1, min(L - 1, int(L * start_t)))
        d = _dir(a, b)
        pos = (a[0] + DV[d][0] * k, a[1] + DV[d][1] * k)
        self.cats.append(dict(pos=pos, dir=d, kind=kind))
        self.routes.append(cells)
        return pos

    def finish(self, start, burrow_pos, decor=()):
        m = self.m
        for p, ch in decor:
            if m.get(*p) == ".":
                m.set(*p, ch)
        m.set(*burrow_pos, "V", margin=0)
        m.set(*start, "A")
        rows = ["".join(r) for r in m.g]
        orient = {}
        under = {}
        g = [list(r) for r in rows]
        for c in self.cats:
            x, y = c["pos"]
            assert g[y][x] == "t", f"cat start {c['pos']} not on plain tape"
            g[y][x] = c["kind"]
            orient[(x, y)] = "LURD"[c["dir"] - 1]
            under[(x, y)] = "t"
        return ["".join(r) for r in g], orient, under


def _dir(a, b):
    dx, dy = (b[0] > a[0]) - (b[0] < a[0]), (b[1] > a[1]) - (b[1] < a[1])
    return next(k for k, v in DV.items() if v == (dx, dy))


def night_shift():
    L = Level(24, 14, 1)
    m = L.m
    L.row(4, 6, 18)
    L.row(9, 6, 10)
    L.row(9, 14, 18)
    L.put([(9, 4), (16, 4), (8, 9), (16, 9)], "G")
    L.route([(4, 3), (20, 3), (20, 5), (4, 5)], start=0, start_t=0.4)
    L.put([(12, 7), (12, 8), (2, 7), (21, 8), (21, 9)], "c")
    deco = [((1, 1), "k"), ((22, 1), "k"), ((22, 12), "P"), ((12, 10), "M"), ((12, 11), "m"), ((6, 11), "v"),
            ((7, 11), "v"), ((17, 7), "m"), ((1, 12), "P")]
    return L.finish((2, 2), (1, 10), deco), "Bump the blinking racks for GPUs; dodge the night-shift sysadmin."


def cold_aisle():
    L = Level(28, 16, 2)
    for y in (3, 7, 11):
        L.row(y, 4, 12)
        L.row(y, 15, 23)
    L.put([(6, 3), (20, 3), (10, 7), (17, 7), (5, 11), (22, 11)], "G")
    L.route([(2, 5), (25, 5)], loop=False, start=0, start_t=0.3)
    L.route([(25, 9), (2, 9)], loop=False, start=0, start_t=0.6)
    L.route([(13, 10), (13, 14)], loop=False, start=0, start_t=0.5)
    L.route([(13, 1), (13, 4)], loop=False, start=0, start_t=0.5)
    L.put([(1, 6), (26, 6), (26, 10), (1, 10), (7, 13), (19, 1), (20, 13)], "c")
    deco = [((1, 1), "k"), ((26, 1), "k"), ((26, 14), "P"), ((14, 8), "m"), ((3, 13), "v"), ((24, 13), "v"),
            ((10, 13), "M"), ((17, 1), "v")]
    return L.finish((1, 14), (14, 15), deco), "Three sysadmins pace the cold aisles. Time your dash between them."


def security_desk():
    L = Level(30, 18, 3)
    m = L.m
    m.rect(9, 6, 20, 11, "w", filled=False)      # the GPU vault
    for p in [(14, 6), (15, 6), (14, 11), (15, 11)]:
        m.set(*p, ".")
    L.row(8, 11, 13)
    L.row(9, 16, 18)
    L.put([(12, 8), (17, 9)], "G")
    L.route([(7, 4), (22, 4), (22, 13), (7, 13)], start=1, start_t=0.5, kind="H")
    L.row(2, 3, 8)
    L.row(2, 21, 26)
    L.row(15, 3, 8)
    L.row(15, 21, 26)
    L.put([(5, 2), (24, 15)], "G")
    L.route([(2, 1), (2, 16)], loop=False, start=0, start_t=0.7)
    L.put([(10, 7), (19, 10), (4, 9), (26, 8), (14, 1), (15, 16), (1, 5)], "c")
    deco = [((1, 1), "k"), ((28, 1), "k"), ((28, 16), "P"), ((10, 9), "M"), ((15, 8), "m"), ((14, 9), "v"),
            ((26, 4), "v"), ((5, 10), "P"), ((25, 11), "m")]
    return L.finish((27, 2), (29, 9), deco), ("A security cat circles the GPU vault. If it spots you it gives "
                                              "chase: hide in the cables until it gives up.")


def hot_aisle():
    L = Level(32, 20, 4)
    m = L.m
    for x in (5, 10, 21, 26):
        L.col(x, 3, 8)
        L.col(x, 11, 16)
    m.rect(14, 7, 17, 12, "w", filled=False)       # hot-aisle containment pod
    m.set(15, 7, ".")
    m.set(16, 12, ".")
    L.put([(15, 9), (16, 10)], "R")
    L.put([(15, 10), (5, 5), (10, 14), (21, 4), (26, 13), (16, 9)], "G")
    L.route([(7, 2), (24, 2), (24, 17), (7, 17)], start=0, start_t=0.3, kind="H")
    L.route([(12, 5), (19, 5), (19, 14), (12, 14)], start=2, start_t=0.5, kind="J")
    L.route([(3, 3), (3, 16)], loop=False, start=0, start_t=0.5)
    L.route([(28, 16), (28, 3)], loop=False, start=0, start_t=0.5)
    L.put([(8, 9), (8, 10), (23, 9), (23, 10), (15, 1), (16, 18), (1, 1), (30, 18), (13, 9)], "c")
    deco = [((1, 18), "k"), ((30, 1), "k"), ((15, 4), "M"), ((16, 15), "m"), ((7, 6), "v"), ((24, 13), "v"),
            ((1, 8), "P"), ((30, 7), "P"), ((12, 3), "m")]
    return L.finish((1, 10), (31, 10), deco), ("Two security cats, two sysadmins and a containment pod in the "
                                               "middle of the hot aisle.")


def plateforme():
    L = Level(36, 22, 5)
    m = L.m
    for y in (3, 18):
        L.row(y, 4, 14)
        L.row(y, 21, 31)
    for x in (4, 31):
        L.col(x, 7, 14)
    m.rect(12, 7, 23, 14, "w", filled=False)       # the training cluster hall
    for p in [(17, 7), (18, 7), (17, 14), (18, 14), (12, 10), (23, 11)]:
        m.set(*p, ".")
    L.row(9, 14, 16)
    L.row(12, 19, 21)
    L.put([(20, 9), (15, 12)], "R")
    L.put([(15, 9), (20, 12), (6, 3), (29, 18), (4, 10), (31, 11), (12, 18), (23, 3)], "G")
    L.route([(10, 5), (25, 5), (25, 16), (10, 16)], start=0, start_t=0.5, kind="H")
    L.route([(2, 1), (33, 1)], loop=False, start=0, start_t=0.2, kind="J")
    L.route([(2, 20), (33, 20)], loop=False, start=0, start_t=0.7, kind="K")
    L.route([(14, 8), (21, 8)], loop=False, start=0, start_t=0.5)
    L.route([(21, 13), (14, 13)], loop=False, start=0, start_t=0.5)
    L.put([(1, 7), (1, 14), (34, 7), (34, 14), (17, 10), (18, 11), (8, 10), (27, 11), (17, 2), (18, 19)], "c")
    deco = [((1, 1), "k"), ((34, 1), "k"), ((1, 20), "P"), ((34, 20), "P"), ((17, 4), "M"), ((18, 17), "M"),
            ((7, 8), "m"), ((28, 13), "m"), ((14, 11), "v"), ((21, 10), "v"), ((9, 13), "v"), ((26, 8), "v")]
    return L.finish((2, 11), (35, 10), deco), ("La Plateforme: eight GPUs, three security cats and two sysadmins "
                                               "guarding the training cluster.")


LEVELS = [("Night Shift", night_shift), ("Cold Aisle", cold_aisle), ("The Security Desk", security_desk),
          ("Hot Aisle Containment", hot_aisle), ("La Plateforme", plateforme)]


# ----------------------------------------------------------------------------- simulation + planner
def parse(rows, orient):
    cats = []
    beds = {}
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == "C" or ch in HUNTERS:
                cats.append(dict(pos=(x, y), dir=LETTER[orient[(x, y)]], mv=0, kind=ch,
                                 sight=HUNT_SIGHT if ch in HUNTERS else CAT_SIGHT))
            if ch in BED_CH:
                beds[BED_CH[ch]] = (x, y)
    static = [list(r.replace("C", "t").replace("H", "t").replace("J", "t").replace("K", "t").replace("A", "."))
              for r in rows]
    return cats, static


def simulate(static, cats, ticks):
    """Patrol replica (no alerts). Returns per tick t (state after t player steps) the cat cells and the
    watched cells. Frame 0 = the initial state."""
    H, W = len(static), len(static[0])
    cs = [dict(c) for c in cats]
    frames = [(frozenset(c["pos"] for c in cs), frozenset())]
    for t in range(ticks):
        for c in cs:
            if c["mv"] == 1:
                c["mv"] = 0
                d = DV[c["dir"]]
                n = (c["pos"][0] + d[0], c["pos"][1] + d[1])
                ch = static[n[1]][n[0]]
                others = {o["pos"] for o in cs if o is not c}
                assert ch in ROUTE and n not in others, f"cat at {c['pos']} blocked by {ch!r} at {n} (t={t})"
                c["pos"] = n
                if ch in MARK:
                    c["dir"] = MARK[ch]
                elif ch in BED_CH:
                    c["dir"] = 3
            else:
                c["mv"] = 1
        watched = set()
        for c in cs:
            d = DV[c["dir"]]
            for r in range(1, c["sight"] + 1):
                x, y = c["pos"][0] + d[0] * r, c["pos"][1] + d[1] * r
                if not (0 <= x < W and 0 <= y < H) or static[y][x] in SOLID:
                    break
                watched.add((x, y))
        frames.append((frozenset(c["pos"] for c in cs), frozenset(watched)))
    return frames


def plan(rows, orient, horizon=6000):
    cats, static = parse(rows, orient)
    frames = simulate(static, cats, horizon)
    start = next((x, y) for y, r in enumerate(rows) for x, ch in enumerate(r) if ch == "A")
    gpus = {(x, y) for y, r in enumerate(rows) for x, ch in enumerate(r) if ch == "G"}
    burrow = next((x, y) for y, r in enumerate(rows) for x, ch in enumerate(r) if ch == "V")
    H, W = len(static), len(static[0])
    walk = lambda p: static[p[1]][p[0]] in FLOORISH or static[p[1]][p[0]] == "c"

    def safe(p, t):
        pos, watched = frames[t]
        return p not in pos and (p not in watched or static[p[1]][p[0]] == "c")

    actions = []
    pos, t, carry = start, 0, 0
    left = set(gpus)
    while left or carry:
        targets = set()
        if carry < CAP:
            targets |= left
        if carry:
            targets.add(burrow)
        # BFS in space-time: node (p, t); edges = wait / move; goal = bump into an adjacent target
        prev = {(pos, t): None}
        layer = [(pos, t)]
        found = None
        while layer and not found:
            nxt = []
            for p, tt in layer:
                if tt + 1 >= horizon:
                    continue
                for a, (dx, dy) in [(0, (0, 0))] + list(DV.items()):
                    q = (p[0] + dx, p[1] + dy)
                    if a and q in targets:
                        if safe(p, tt + 1):
                            found = ((p, tt), a, q)
                            break
                        continue
                    if a and (not walk(q) or q in frames[tt][0]):
                        continue
                    if not safe(q, tt + 1) or (q, tt + 1) in prev:
                        continue
                    prev[(q, tt + 1)] = ((p, tt), a)
                    nxt.append((q, tt + 1))
                if found:
                    break
            layer = nxt
        assert found, f"planner: no safe route from {pos} at t={t} (carry {carry}, left {len(left)})"
        node, a, q = found
        leg = [a]
        while prev[node] is not None:
            node, a2 = prev[node]
            leg.append(a2)
        leg.reverse()
        actions += leg
        t = found[0][1] + 1
        pos = found[0][0]
        if q == burrow:
            carry = 0
        else:
            carry += 1
            left.discard(q)
            static[q[1]][q[0]] = "Z"
    return actions, len(gpus), cats


# ----------------------------------------------------------------------------- GDY
WALLS = ["wall", "rack", "rack_gpu", "rack_empty", "burrow", "coolant", "plant", "logo_sign"]
FLOOR_OBJS = ["tape", "mark_l", "mark_u", "mark_r", "mark_d", "logo_floor", "vent", "bed1", "bed2", "bed3"]
FLOOR = "[_empty, " + ", ".join(FLOOR_OBJS) + "]"
HCATS = ["hcat1", "hcat2", "hcat3"]
CATS = ["cat"] + HCATS
SIGHT_BLOCK = "[" + ", ".join(WALLS + CATS) + "]"
MARK_OBJ = {1: "mark_l", 2: "mark_u", 3: "mark_r", 4: "mark_d"}

ABS4 = lambda k: "\n".join(
    f"        {i}: {{Description: {n}, VectorToDest: [{DV[i][0] * k}, {DV[i][1] * k}], "
    f"MetaData: {{d: {i}, tile: {TILE[i]}, atile: {TILE[i] + 4}}}}}"
    for i, n in ((1, "Left"), (2, "Up"), (3, "Right"), (4, "Down")))


def look(k):
    hc = "\n".join(f'''      - Src: {{Object: {h}, Preconditions: [{{eq: [dst.hidden, 0]}}], Commands: [{{set: [clear, 0]}}, {{set: [mode, 1]}}, {{set: [alert, {ALERT}]}}, {{set: [mv, 0]}}]}}
        Dst: {{Object: bufo}}''' for h in HCATS)
    return f'''  - Name: look{k}
    InputMapping:
      Internal: true
      Inputs:
{ABS4(k)}
    Behaviours:
      - Src: {{Object: [{", ".join(CATS)}], Commands: [{{set: [clear, 0]}}]}}
        Dst: {{Object: {SIGHT_BLOCK}}}
      - Src: {{Object: cat, Preconditions: [{{eq: [dst.hidden, 0]}}], Commands: [{{set: [clear, 0]}}]}}
        Dst: {{Object: bufo, Commands: [{{remove: true}}]}}
{hc}
'''


def looks(n):
    out = "            - set: [clear, 1]\n            - exec: {Action: look1, ActionId: dir}\n"
    for k in range(2, n + 1):
        out += f"            - if: {{Conditions: {{eq: [clear, 1]}}, OnTrue: [{{exec: {{Action: look{k}, ActionId: dir}}}}]}}\n"
    return out


def walk_cmds():
    """Patrol step every 2nd tick (shared by sysadmins and patrolling security cats)."""
    return "{if: {Conditions: {eq: [mv, 1]}, OnTrue: [{set: [mv, 0]}, {exec: {Action: gstep, ActionId: dir}}], OnFalse: [{set: [mv, 1]}]}}"


def hunter_patrol(h, k):
    impass = "[" + ", ".join(WALLS + ["cable"] + [c for c in CATS if c != h]) + "]"
    return f'''      - Src:
          Object: {h}
          Commands:
            - if: {{Conditions: {{eq: [mode, 0]}}, OnTrue: [{walk_cmds()}]}}
            - if:
                Conditions: {{eq: [mode, 2]}}
                OnTrue:
                  - if: {{Conditions: {{eq: [mv, 1]}}, OnTrue: [{{set: [mv, 0]}}, {{exec: {{Action: hstep, Search: {{TargetObjectName: bed{k}, ImpassableObjects: {impass}}}}}}}], OnFalse: [{{set: [mv, 1]}}]}}
            - if:
                Conditions: {{eq: [mode, 1]}}
                OnTrue:
                  - decr: alert
                  - exec: {{Action: hstep, Search: {{TargetObjectName: bufo, ImpassableObjects: {impass}}}}}
                  - if: {{Conditions: {{lte: [alert, 0]}}, OnTrue: [{{set: [mode, 2]}}, {{set: [mv, 0]}}]}}
{looks(HUNT_SIGHT)}            - exec: {{Action: patrol, ActionId: 1, Delay: 1}}
        Dst: {{Object: {h}}}
'''


def actions():
    mv_floor = f'''      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {{if: {{Conditions: {{gt: [carry, 0]}}, OnTrue: [{{set_tile: meta.gfacing}}]}}}}, {{mov: _dest}}, {{set: [hidden, 0]}}]}}
        Dst: {{Object: {FLOOR}}}'''
    face = "{set_tile: meta.facing}, {if: {Conditions: {gt: [carry, 0]}, OnTrue: [{set_tile: meta.gfacing}]}}"
    marks = "\n".join(f'''      - Src: {{Object: [{", ".join(CATS)}], Commands: [{{mov: _dest}}, {{set: [dir, {d}]}}, {{set_tile: {TILE[d]}}}]}}
        Dst: {{Object: {o}}}''' for d, o in MARK_OBJ.items())
    beds = "\n".join(f'''      - Src: {{Object: [{", ".join(CATS)}], Commands: [{{mov: _dest}}, {{set: [dir, 3]}}, {{set_tile: 1}}]}}
        Dst: {{Object: bed{k}}}''' for k in (1, 2, 3))
    hsteps = "\n".join(f'''      - Src:
          Object: {h}
          Commands:
            - mov: _dest
            - set: [dir, meta.d]
            - set_tile: meta.tile
            - if: {{Conditions: {{eq: [mode, 1]}}, OnTrue: [{{set_tile: meta.atile}}]}}
        Dst: {{Object: [_empty, tape, mark_l, mark_u, mark_r, mark_d, logo_floor, vent, {", ".join(b for b in ("bed1", "bed2", "bed3") if b != f"bed{k}")}]}}
      - Src:
          Object: {h}
          Commands:
            - mov: _dest
            - set: [dir, meta.d]
            - set_tile: meta.tile
            - if: {{Conditions: {{eq: [mode, 1]}}, OnTrue: [{{set_tile: meta.atile}}]}}
            - if: {{Conditions: {{eq: [mode, 2]}}, OnTrue: [{{set: [mode, 0]}}, {{set: [mv, 0]}}, {{set: [dir, 3]}}, {{set_tile: 1}}]}}
        Dst: {{Object: bed{k}}}
      - Src: {{Object: {h}, Preconditions: [{{eq: [dst.hidden, 0]}}], Commands: [{{set: [dir, meta.d]}}]}}
        Dst: {{Object: bufo, Commands: [{{remove: true}}]}}''' for k, h in enumerate(HCATS, 1))
    retile = "\n".join(f"            - if: {{Conditions: {{eq: [dir, {d}]}}, OnTrue: [{{set_tile: {TILE[d]}}}]}}" for d in DV)
    hpatrols = "".join(hunter_patrol(h, k) for k, h in enumerate(HCATS, 1))
    anim_logo, _ = M.animate("logo_sign", 4, 3)
    anim_rack, _ = M.animate("rack_gpu", 2, 3)
    return f'''
Actions:
{C.face_action("bufo")}
  # Stand still for one tick (Space).
  - Name: wait
    InputMapping:
      Inputs:
        1: {{Description: Wait, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: bufo, Commands: [{{nop: []}}]}}
        Dst: {{Object: bufo}}

  # Bufo: absolute 4-way move. Walk into a blinking rack to steal its GPU (max {CAP}), into the burrow to
  # deliver. Render tiles 0..3 = facing, 4..7 = facing while carrying.
  - Name: move
    InputMapping:
      Inputs:
        1: {{Description: Left, VectorToDest: [-1, 0], MetaData: {{facing: 3, gfacing: 7}}}}
        2: {{Description: Up, VectorToDest: [0, -1], MetaData: {{facing: 0, gfacing: 4}}}}
        3: {{Description: Right, VectorToDest: [1, 0], MetaData: {{facing: 1, gfacing: 5}}}}
        4: {{Description: Down, VectorToDest: [0, 1], MetaData: {{facing: 2, gfacing: 6}}}}
    Behaviours:
{mv_floor}
      - Src: {{Object: bufo, Commands: [{face}, {{mov: _dest}}, {{set: [hidden, 1]}}]}}
        Dst: {{Object: cable}}
      - Src: {{Object: bufo, Preconditions: [{{lt: [src.carry, {CAP}]}}], Commands: [{{incr: carry}}, {{set_tile: meta.gfacing}}, {{reward: 1}}]}}
        Dst: {{Object: rack_gpu, Commands: [{{change_to: rack_empty}}]}}
      - Src: {{Object: bufo, Preconditions: [{{gte: [src.carry, {CAP}]}}], Commands: [{face}]}}
        Dst: {{Object: rack_gpu}}
      - Src: {{Object: bufo, Preconditions: [{{gt: [src.carry, 0]}}], Commands: [{{sub: [need, carry]}}, {{reward: 3}}, {{set: [carry, 0]}}, {{set_tile: meta.facing}}]}}
        Dst: {{Object: burrow}}
      - Src: {{Object: bufo, Preconditions: [{{eq: [src.carry, 0]}}], Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: burrow}}
      - Src: {{Object: bufo, Commands: [{face}]}}
        Dst: {{Object: [wall, rack, rack_empty, coolant, plant, logo_sign]}}
      - Src: {{Object: bufo, Commands: [{{remove: true}}]}}
        Dst: {{Object: [{", ".join(CATS)}]}}

  # Every GPU rack adds itself to `need` when the level starts.
  - Name: count_gpu
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: rack_gpu, Commands: [{{incr: need}}, {{exec: {{Action: rack_gpu_anim, ActionId: 1}}}}]}}
        Dst: {{Object: rack_gpu}}

  # Cats start from their level orientation (C[R]): dir = 1 left, 2 up, 3 right, 4 down.
  - Name: cat_init
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Init, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: [{", ".join(CATS)}]
          Commands:
            - if: {{Conditions: {{eq: [_dx, -1]}}, OnTrue: [{{set: [dir, 1]}}]}}
            - if: {{Conditions: {{eq: [_dy, -1]}}, OnTrue: [{{set: [dir, 2]}}]}}
            - if: {{Conditions: {{eq: [_dx, 1]}}, OnTrue: [{{set: [dir, 3]}}]}}
            - if: {{Conditions: {{eq: [_dy, 1]}}, OnTrue: [{{set: [dir, 4]}}]}}
            - exec: {{Action: retile, ActionId: 1}}
            - exec: {{Action: patrol, ActionId: 1, Delay: 1}}
        Dst: {{Object: [{", ".join(CATS)}]}}

  - Name: retile
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Retile, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: [{", ".join(CATS)}]
          Commands:
{retile}
        Dst: {{Object: [{", ".join(CATS)}]}}

  # Cat brain, every tick. Sysadmin: step every 2nd tick, look 3 ahead (seeing bufo = caught).
  # Security cats: mode 0 patrol / 1 chase bufo with A* every tick (alert countdown) / 2 walk home (A*).
  - Name: patrol
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: cat
          Commands:
            - {walk_cmds()}
{looks(CAT_SIGHT)}            - exec: {{Action: patrol, ActionId: 1, Delay: 1}}
        Dst: {{Object: cat}}
{hpatrols}
  # One patrol step along the tape; chevrons turn the cat, beds point it along its route.
  - Name: gstep
    InputMapping:
      Internal: true
      Inputs:
{ABS4(1)}
    Behaviours:
      - Src: {{Object: [{", ".join(CATS)}], Commands: [{{mov: _dest}}]}}
        Dst: {{Object: [_empty, tape, logo_floor, vent]}}
{marks}
{beds}
      - Src: {{Object: [{", ".join(CATS)}], Commands: [{{nop: []}}]}}
        Dst: {{Object: bufo, Commands: [{{remove: true}}]}}

  # A security cat's A* step (chasing bufo or heading home); bufo hiding in cables can't be caught.
  - Name: hstep
    InputMapping:
      Internal: true
      Inputs:
{ABS4(1)}
    Behaviours:
{hsteps}

{look(1)}{look(2)}{look(3)}{look(4)}{anim_logo}{anim_rack}'''


def objects():
    v = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: {iv}\n" for n, iv in names)
    init = "    InitialActions:\n      - Action: cat_init\n        ActionId: 1\n"
    bufo_extra = v([("carry", 0), ("hidden", 0)]) + C.FACE_INITIAL_ACTION
    cat_vars = [("dir", 3), ("mv", 0), ("clear", 1)]
    _, logo_init = M.animate("logo_sign", 4, 3)
    rack_init = "    InitialActions:\n      - Action: count_gpu\n        ActionId: 1\n"
    out = [
        C.obj("wall", "w", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("tape", "t", 0, C.img(tape()), "[0.8, 0.7, 0.2]", "square", 0.3),
        C.obj("mark_u", "u", 0, C.img(tape("u")), "[0.9, 0.8, 0.2]", "triangle", 0.4),
        C.obj("mark_r", "r", 0, C.img(tape("r")), "[0.9, 0.8, 0.2]", "triangle", 0.4),
        C.obj("mark_d", "d", 0, C.img(tape("d")), "[0.9, 0.8, 0.2]", "triangle", 0.4),
        C.obj("mark_l", "l", 0, C.img(tape("l")), "[0.9, 0.8, 0.2]", "triangle", 0.4),
        C.obj("logo_floor", "m", 0, C.img(M.mistral_logo()), "[1.0, 0.5, 0.0]", "square", 0.6),
        C.obj("vent", "v", 0, C.img(vent()), "[0.4, 0.42, 0.5]", "square", 1.0),
        C.obj("cable", "c", 0, C.img(M.cable()), "[0.2, 0.5, 0.95]", "circle", 0.9),
        C.obj("bed1", "p", 0, C.img(cat_bed()), "[0.8, 0.3, 0.2]", "circle", 0.7),
        C.obj("bed2", "q", 0, C.img(cat_bed()), "[0.8, 0.3, 0.2]", "circle", 0.7),
        C.obj("bed3", "o", 0, C.img(cat_bed()), "[0.8, 0.3, 0.2]", "circle", 0.7),
        C.obj("rack", "R", 1, C.img(M.server_rack("idle")), "[0.25, 0.25, 0.3]", "square", 1.0),
        C.obj("rack_gpu", "G", 1, C.imgs(M.rack_frames()), "[0.2, 0.9, 0.3]", "square", 1.0, rack_init, tiles=2),
        C.obj("rack_empty", None, 1, C.img(M.server_rack("empty")), "[0.1, 0.1, 0.12]", "square", 1.0),
        C.obj("burrow", "V", 1, C.img(burrow()), "[0.6, 0.4, 0.2]", "circle", 1.0),
        C.obj("coolant", "k", 1, C.img(M.coolant()), "[0.3, 0.8, 0.9]", "square", 0.8),
        C.obj("plant", "P", 1, C.img(plant()), "[0.2, 0.7, 0.3]", "circle", 0.8),
        C.obj("logo_sign", "M", 1, C.imgs(M.mistral_logo_spin()), "[1.0, 0.6, 0.0]", "square", 0.9, logo_init, tiles=4),
        C.obj("cat", "C", 2, C.imgs(M.cat("sysadmin")), "[0.7, 0.7, 0.75]", "triangle", 0.9, v(cat_vars) + init, tiles=4),
    ]
    hc_sprites = C.imgs(M.cat("guard") + alert_cat())
    for k, (h, ch) in enumerate(zip(HCATS, "HJK"), 1):
        out.append(C.obj(h, ch, 2, hc_sprites, "[0.85, 0.5, 0.2]", "triangle", 0.9,
                         v(cat_vars + [("mode", 0), ("alert", 0)]) + init, tiles=8))
    out.append(C.obj("bufo", "A", 2, C.imgs(M.bufo("plain") + M.bufo("gpu")), "[0.4, 0.7, 0.25]", "triangle", 0.9,
                     bufo_extra, tiles=8))
    return "\nObjects:\n" + "".join(out)


def level_block(name, blurb, rows, orient, under):
    lines = []
    for y, r in enumerate(rows):
        toks = []
        for x, c in enumerate(r):
            o = f"[{orient[(x, y)]}]" if (x, y) in orient else ""
            assert not (o and x == len(r) - 1)
            g = under.get((x, y))
            toks.append(f"{g}/{c}{o}" if g else c + o)
        lines.append(" ".join(t.ljust(6) for t in toks).rstrip())
    return f"    # {name} | {blurb}\n    - |\n" + "\n".join("      " + ln for ln in lines) + "\n"


def main():
    variables = "    - Name: need\n      InitialValue: 0\n"
    hdr = C.header("Bufos GPU Heist", """
        Stealth in a Mistral datacenter. Bufo sneaks between the server racks stealing GPUs: walk into a
        blinking rack to grab its GPU (carry up to 2), walk into the bufo burrow to deliver. Deliver every
        GPU to win. Sysadmin cats patrol the taped routes and see 3 tiles ahead - if one sees you, you're
        caught. Security cats (caps) see 4 tiles and, when they spot you, chase you with A* for 12 ticks
        before walking back to their cat bed. Cable bundles hide you and cats can't follow you in.
        """.replace("        ", ""), "bufo", ["eq: [need, 0]"], ["eq: [bufo:count, 0]"],
        background=M.datacenter_floor(), follow=(17, 11), variables=variables)
    parts = [hdr]
    sols = []
    infos = []
    for i, (name, fn) in enumerate(LEVELS):
        (rows, orient, under), blurb = fn()
        C.check_rect(name, rows)
        start = C.find(rows, "A")[0]
        r = C.reach(rows, start, lambda c, p: c in FLOORISH or c in "cCHJK")
        for t in C.find(rows, "G") + C.find(rows, "V"):
            assert any((t[0] + dx, t[1] + dy) in r for dx, dy in DV.values()), f"{name}: {t} unreachable"
        assert sum(r.count("A") for r in rows) == 1 and sum(r.count("V") for r in rows) == 1, name
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
        acts, ngpu, cats = plan(rows, orient)
        sols.append({"seed": 1, "actions": [["wait", 1] if a == 0 else ["move", a] for a in acts]})
        nh = sum(c["kind"] in HUNTERS for c in cats)
        infos.append((name, len(rows[0]), len(rows), ngpu, len(cats) - nh, nh, len(acts)))
        print(f"{i}. {name:22s} {len(rows[0])}x{len(rows)} gpus={ngpu} sysadmins={len(cats) - nh} security={nh} "
              f"planner run={len(acts)} ticks", file=sys.stderr)
        parts.append(level_block(name, blurb, rows, orient, under))
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Bufo's GPU Heist",
        "tagline": "Sneak past the sysadmin cats and steal every GPU in the datacenter.",
        "description": "Bufo is on a heist in a Mistral datacenter. Bump blinking racks to grab GPUs (two at a time) "
                       "and carry them back to the burrow. Sysadmin cats patrol the taped routes, and security cats "
                       "chase you with A* once they spot you. Hide in cable bundles until they give up.",
        "tags": ["meme", "stealth", "timing", "pathfinding"],
        "difficulty": "medium",
        "original": "",
        "accent": "#84cc16",
        "icon": "🐸",
        "controls": {"move": "move", "extra": [{"key": " ", "label": "Wait", "action": "wait"}]},
        "hud": [
            {"label": "GPUs to steal", "kind": "global", "variable": "need", "tone": "emerald"},
            {"label": "Carrying", "kind": "var", "object": "bufo", "variable": "carry", "tone": "lime"},
            {"label": "Hidden", "kind": "var", "object": "bufo", "variable": "hidden", "tone": "sky"},
        ],
        "win_text": "Every GPU is in the burrow. Bufo's cluster is now the biggest in the swamp.",
        "lose_text": "Caught by a sysadmin cat! Bufo is escorted out of the datacenter.",
    })
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps({"format": 2, "levels": sols}))
    print(f"wrote {SOLUTIONS.relative_to(C.ROOT)}", file=sys.stderr)
    if "--check" in sys.argv:
        r = subprocess.run(["node", str(HERE / "replay_solutions.mjs"), GID], capture_output=True, text=True)
        print(r.stdout, file=sys.stderr)
        assert r.returncode == 0, "replay failed"



if __name__ == "__main__":
    main()
