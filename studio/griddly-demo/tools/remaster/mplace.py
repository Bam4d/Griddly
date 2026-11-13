#!/usr/bin/env python3
"""m/place: bufo paints office pixel art, one mayo per pixel (meme game, phase 4).

Inspired by the office pixel canvas where 1 mayo = 1 pixel. Each level has a canvas showing a target
image as faint "ghost" pixels. Walk onto a paint bucket to pick its colour (the bufo's brush blob
shows it), then `paint` the pixel in front of you (directional, Space): it costs 1 mayo. Mayo jars
refill you (+JAR each). Rival troll bufos patrol straight lanes (they turn back at anything solid,
including you) and paint every canvas pixel they step on in their own colour; throw mayo at a
troll (paint it) to ban it for 1 mayo.

Win: every canvas pixel matches its ghost (global `wrong` == 0).
Lose: the mayo you hold plus the mayo left in jars (global `supply`) is less than the pixels still
wrong.

Rules in the GDY:
  * A canvas pixel is one Z=0 object `c_<target>_<paint>` (paint `none` = unpainted ghost); painting
    `change_to`s it. Ghosts count themselves into `wrong` at reset; paint rules adjust `wrong`.
  * Jars add JAR to `supply` at reset (InitialActions), picking one up moves it into `mayo`.
  * Trolls: internal `troll` action (1-4 inputs, MetaData d) re-execs itself every TROLL_PERIOD ticks
    with ActionId = its `dir` variable; blocked -> reverse.

Validation: an exact Python simulation (Sim) of these rules plus a greedy planner (ban trolls, fetch
mayo when dry, paint nearest pixels of the held colour, switch to the nearest useful bucket) must win
every level within its mayo budget; the run is written as a format-2 solution and replayed in the
engine by replay_solutions.mjs. `--diff` compares Sim against the engine (office_dump.mjs) step by step.

Map chars: w wall, Q rack, L logo, P plant, A bufo, j mayo jar, e blank canvas (not part of the image),
ghost pixels r d o a y k g h (red redorange orange amber yellow black green white), buckets
R D O U Y K G H (same order), trolls t/v (black, start right/down), x/z (green, start right/down).
"""
from __future__ import annotations

import collections
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402
import office_kit as K  # noqa: E402

GID = "mplace"
JAR = 6
TROLL_PERIOD = 2
COLOURS = ["red", "redorange", "orange", "amber", "yellow", "black", "green", "white"]
RGB = {"red": (225, 40, 40), "redorange": M.P["mistral_redorange"][:3], "orange": (255, 130, 5),
       "amber": M.P["mistral_amber"][:3], "yellow": (255, 216, 0), "black": (40, 38, 46),
       "green": (70, 180, 80), "white": (238, 236, 228)}
GHOST_CH = dict(zip(COLOURS, "rdoaykgh"))
BUCKET_CH = dict(zip(COLOURS, "RDOUYKGH"))
CH_GHOST = {v: k for k, v in GHOST_CH.items()}
CH_BUCKET = {v: k for k, v in BUCKET_CH.items()}
CI = {c: i + 1 for i, c in enumerate(COLOURS)}  # colour index (avatar `col`; 0 = no colour)
TROLLS = {"t": ("black", 3), "v": ("black", 4), "x": ("green", 3), "z": ("green", 4)}
DIRS = K.DIRS
REV = {1: 3, 2: 4, 3: 1, 4: 2}
SOLID = set("wQLP")


def cell_name(t, p):
    return f"c_{t}_{p}"


def troll_name(ch):
    col, d = TROLLS[ch]
    return f"troll_{col}_{'h' if d == 3 else 'v'}"


PIXELS = [f"px_{t}" for t in COLOURS]
TROLL_NAMES = sorted({troll_name(c) for c in TROLLS})
BUCKETS = [f"bucket_{c}" for c in COLOURS]

# ------------------------------------------------------------------------------ pictures
HEART = [".rr.rr.",
         "rhhrrrr",
         "rhrrrrr",
         ".rrrrr.",
         "..rrr..",
         "...r..."]
BUFO = [".gg...gg.",
        "ghkg.ghkg",
        "ggggggggg",
        "gkgggggkg",
        "ggkkkkkgg",
        ".ggggggg."]
CAT = ["o.......o",
       "oo.....oo",
       "ooooooooo",
       "ookoookoo",
       "ooooroooo",
       "hookokooh",
       ".ooooooo.",
       "..ooooo.."]
M_ROWS = ["X...X", "XX.XX", "XXXXX", "X.X.X", "X...X"]
M_STRIPE = "yaodr"


def m_logo(sx=2, sy=1):
    out = []
    for r, row in enumerate(M_ROWS):
        line = "".join((M_STRIPE[r] if ch == "X" else ".") * sx for ch in row)
        out += [line] * sy
    return out


def stripe_rows(rows, stripes):
    """Recolour the X cells of `rows` by row index into stripes (len == len(rows))."""
    return ["".join(stripes[i] if ch != "." else "." for ch in r) for i, r in enumerate(rows)]


# ------------------------------------------------------------------------------ levels
class Room:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.g = [["w" if x in (0, w - 1) or y in (0, h - 1) else "." for x in range(w)] for y in range(h)]

    def put(self, x, y, ch):
        assert self.g[y][x] == ".", f"cell {(x, y)} already {self.g[y][x]!r}"
        self.g[y][x] = ch

    def canvas(self, x0, y0, pic, ring=1):
        h, w = len(pic), len(pic[0])
        for y in range(y0 - ring, y0 + h + ring):
            for x in range(x0 - ring, x0 + w + ring):
                self.g[y][x] = "e"
        for dy, row in enumerate(pic):
            for dx, ch in enumerate(row):
                if ch != ".":
                    self.g[y0 + dy][x0 + dx] = ch

    def many(self, ch, pts):
        for x, y in pts:
            self.put(x, y, ch)

    def rows(self):
        return ["".join(r) for r in self.g]


def lvl_heart():
    r = Room(17, 12)
    r.canvas(5, 3, HEART)
    r.put(2, 5, "A")
    r.many("j", [(2, 3), (2, 8), (14, 2), (14, 9), (8, 10)])
    r.put(1, 1, "R")
    r.put(1, 10, "H")
    r.many("Q", [(4, 1), (5, 1), (6, 1)])
    r.many("P", [(15, 1), (15, 10), (1, 5)])
    r.many("L", [(10, 1)])
    return r.rows()


def lvl_bufo():
    r = Room(21, 13)
    r.canvas(6, 3, BUFO)
    r.put(10, 11, "A")
    r.many("j", [(1, 1), (19, 1), (1, 11), (19, 11), (3, 6), (17, 6), (10, 1), (13, 11)])
    r.many("G", [(2, 3)])
    r.many("K", [(18, 3)])
    r.many("H", [(18, 9)])
    r.many("Q", [(4, 1), (5, 1), (14, 1), (15, 1)])
    r.many("P", [(1, 6), (19, 6), (7, 11)])
    r.many("L", [(9, 1), (11, 1)])
    return r.rows()


def lvl_cat():
    r = Room(23, 15)
    r.canvas(7, 3, CAT)
    r.put(11, 13, "A")
    r.put(1, 7, "t")  # black troll patrolling row 7 (the cat's mouth row)
    r.many("j", [(3, 13), (19, 13), (3, 1), (19, 1), (21, 4), (21, 10), (11, 1), (8, 13), (14, 13), (1, 11)])
    r.many("O", [(2, 4)])
    r.many("K", [(20, 6)])
    r.many("H", [(2, 10)])
    r.many("R", [(20, 8)])
    r.many("Q", [(5, 1), (6, 1), (16, 1), (17, 1)])
    r.many("P", [(1, 1), (21, 1), (1, 13), (21, 13)])
    r.many("L", [(9, 1), (13, 1)])
    return r.rows()


def lvl_mistral():
    pic = m_logo(2, 1)
    r = Room(24, 13)
    r.canvas(7, 4, pic)
    r.put(12, 11, "A")
    r.put(1, 5, "t")   # black troll on the amber row
    r.put(15, 1, "v")  # black troll dropping down column 15
    r.many("j", [(1, 1), (22, 1), (1, 11), (22, 11), (4, 6), (19, 9), (12, 1), (8, 11)])
    r.many("Y", [(2, 3)])
    r.many("U", [(21, 3)])
    r.many("O", [(2, 9)])
    r.many("D", [(21, 6)])
    r.many("R", [(17, 11)])
    r.many("Q", [(5, 1), (6, 1), (18, 1), (19, 1)])
    r.many("P", [(1, 6), (22, 6)])
    r.many("L", [(10, 1)])
    return r.rows()


def lvl_finale():
    pic = stripe_rows([row for row in m_logo(2, 2)], "yyaaooddrr")
    r = Room(30, 17)
    r.canvas(10, 3, pic)
    r.put(15, 15, "A")
    r.put(1, 6, "t")    # black troll, row 6
    r.put(1, 10, "x")   # green troll, row 10
    r.put(13, 1, "v")   # black troll, column 13
    r.put(18, 1, "z")   # green troll, column 18
    r.many("j", [(3, 1), (26, 1), (3, 15), (26, 15), (6, 8), (23, 8), (15, 1), (9, 15), (21, 15), (28, 4),
                 (28, 12), (1, 13), (6, 3), (23, 13), (11, 15)])
    r.many("Y", [(3, 4)])
    r.many("U", [(26, 4)])
    r.many("O", [(3, 12)])
    r.many("D", [(26, 12)])
    r.many("R", [(15, 14)])
    r.many("Q", [(6, 1), (7, 1), (8, 1), (21, 1), (22, 1), (23, 1)])
    r.many("P", [(1, 1), (28, 1), (1, 15), (28, 15)])
    r.many("L", [(10, 1), (20, 1)])
    return r.rows()


LEVELS = [
    ("Heart of the Office", "Pick up red at the bucket, grab mayo, paint the ghost heart. 1 mayo = 1 pixel.",
     lvl_heart, 0),
    ("Bufo Portrait", "Three colours, jars all over the office. Plan your mayo runs.", lvl_bufo, 0),
    ("Troll Patrol", "A troll bufo scribbles black over le chaton fat. Throw mayo at it (paint it) to ban it.",
     lvl_cat, 1),
    ("Mistral Stripes", "Five stripes, five buckets, two trolls. Paint the M.", lvl_mistral, 1),
    ("r/mistral", "The big M under attack by four trolls. Ban them, then paint every stripe.", lvl_finale, 1),
]


# ------------------------------------------------------------------------------ simulation
class Sim:
    """Exact model of the GDY rules (see module docstring)."""

    def __init__(self, rows):
        self.rows = rows
        self.cells = {}
        self.jars = set()
        self.buckets = {}
        self.solid = set()
        self.trolls = []
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                p = (x, y)
                if ch in CH_GHOST:
                    self.cells[p] = [CH_GHOST[ch], None]
                elif ch == "j":
                    self.jars.add(p)
                elif ch in CH_BUCKET:
                    self.buckets[p] = CH_BUCKET[ch]
                elif ch in SOLID:
                    self.solid.add(p)
                elif ch == "A":
                    self.pos = p
                elif ch in TROLLS:
                    col, d = TROLLS[ch]
                    self.trolls.append({"col": col, "pos": p, "dir": d, "next": TROLL_PERIOD, "alive": True})
        self.col = None
        self.mayo = 0
        self.supply = JAR * len(self.jars)
        self.wrong = len(self.cells)
        self.tick = 0
        self.paints = 0

    def troll_at(self, p):
        return next((t for t in self.trolls if t["alive"] and t["pos"] == p), None)

    def paint_cell(self, p, colour):
        """Returns the change in `wrong` (cell must exist and colour != current paint)."""
        t, old = self.cells[p]
        self.cells[p][1] = colour
        return (colour != t) - (old != t)

    def step(self, action, d):
        dst = K.add(self.pos, DIRS[d])
        if action == "move":
            if dst in self.solid or self.troll_at(dst):
                pass
            elif dst in self.jars:
                self.jars.discard(dst)
                self.mayo += JAR
                self.pos = dst
            else:
                if dst in self.buckets:
                    self.col = self.buckets[dst]
                self.pos = dst
        elif action == "paint" and self.mayo >= 1:
            tr = self.troll_at(dst)
            if tr:
                tr["alive"] = False
                self.mayo -= 1
                self.supply -= 1
            elif dst in self.cells and self.col and self.cells[dst][1] != self.col:
                self.wrong += self.paint_cell(dst, self.col)
                self.mayo -= 1
                self.supply -= 1
                self.paints += 1
        self.tick += 1
        for tr in self.trolls:
            if tr["alive"] and tr["next"] <= self.tick:
                self.troll_act(tr)
                tr["next"] = self.tick + TROLL_PERIOD
        if self.wrong == 0:
            return "Win"
        if self.supply < self.wrong:
            return "Lose"
        return None

    def troll_act(self, tr):
        dst = K.add(tr["pos"], DIRS[tr["dir"]])
        blocked = (dst == self.pos or dst in self.solid or dst in self.jars or dst in self.buckets
                   or self.troll_at(dst) or self.rows[dst[1]][dst[0]] == "w")
        if blocked:
            tr["dir"] = REV[tr["dir"]]
            return
        if dst in self.cells and self.cells[dst][1] != tr["col"]:
            self.wrong += self.paint_cell(dst, tr["col"])
        tr["pos"] = dst

    def snapshot(self):
        return (self.pos, dict((p, v[1]) for p, v in self.cells.items()), self.mayo, self.supply, self.wrong,
                sorted(t["pos"] for t in self.trolls if t["alive"]))


# ------------------------------------------------------------------------------ planner
def walkable(sim, p):
    return p not in sim.solid and not sim.troll_at(p) and sim.rows[p[1]][p[0]] != "w"


def bfs_first_step(sim, goal_fn):
    """First move id of a shortest walk from the avatar to a cell where goal_fn(cell) is true."""
    start = sim.pos
    if goal_fn(start):
        return 0
    par = {start: None}
    q = collections.deque([start])
    while q:
        c = q.popleft()
        for m, d in DIRS.items():
            n = K.add(c, d)
            if n in par or not walkable(sim, n):
                continue
            par[n] = (c, m)
            if goal_fn(n):
                while par[n][0] != start:
                    n = par[n][0]
                return par[n][1]
            q.append(n)
    return None


def plan(rows, max_steps=4000):
    sim = Sim(rows)
    run = []
    for _ in range(max_steps):
        act = decide(sim)
        if act is None:
            return None, sim, run
        run.append(act)
        res = sim.step(*act)
        if res:
            return res, sim, run
    return None, sim, run


def decide(sim):
    alive = [t for t in sim.trolls if t["alive"]]
    # 1. ban trolls (needs mayo)
    if alive and sim.mayo >= 1:
        for m, d in DIRS.items():
            if sim.troll_at(K.add(sim.pos, d)):
                return ("paint", m)
        tgt = {K.add(t["pos"], d) for t in alive for d in DIRS.values()}
        m = bfs_first_step(sim, lambda p: p in tgt)
        if m:
            return ("move", m)
    # 2. mayo
    if sim.mayo == 0:
        m = bfs_first_step(sim, lambda p: p in sim.jars)
        return ("move", m) if m else None
    todo = {p for p, (t, cur) in sim.cells.items() if t != cur}
    need = collections.Counter(sim.cells[p][0] for p in todo)
    # 3. paint with the held colour
    if sim.col and need[sim.col]:
        for m, d in DIRS.items():
            n = K.add(sim.pos, d)
            if n in todo and sim.cells[n][0] == sim.col and not sim.troll_at(n):
                return ("paint", m)
        targets = {p for p in todo if sim.cells[p][0] == sim.col}
        m = bfs_first_step(sim, lambda p: any(K.add(p, d) in targets for d in DIRS.values()))
        if m:
            return ("move", m)
    # 4. go to the nearest bucket of a colour still needed
    m = bfs_first_step(sim, lambda p: p in sim.buckets and need[sim.buckets[p]] > 0 and sim.buckets[p] != sim.col)
    return ("move", m) if m else None


# ------------------------------------------------------------------------------ art
def rgba(c, a=255):
    return tuple(c[:3]) + (a,)


def ghost_sprite(t):
    base = (214, 214, 220)
    col = RGB[t]
    px = M.blank(rgba(K.blend(col, base, 0.38)))
    edge = rgba(K.blend(col, base, 0.6))
    for i in range(24):
        for (x, y) in ((i, 0), (0, i), (i, 23), (23, i)):
            px[y][x] = edge
    for y in range(3, 24, 6):  # dotted "ghost" grid
        for x in range(3, 24, 6):
            px[y][x] = edge
    return M.save(f"mplace-ghost-{t}", px)


def painted_sprite(t, p):
    px = K.load(M.pixel_canvas("#%02x%02x%02x" % tuple(RGB[p])))
    if p != t:  # wrong: the target colour peeks out of the top-left corner + a little cross
        g = rgba(RGB[t])
        for y in range(9):
            for x in range(9 - y):
                px[y][x] = g if x + y < 8 else M.P["outline"]
        for i in range(5):
            for x, y in ((17 + i, 17 + i), (21 - i, 17 + i)):
                px[y][x] = (255, 255, 255, 255) if p != "white" else (40, 38, 46, 255)
    return M.save(f"mplace-px-{t}-{p}", px)


def blank_canvas_sprite():
    px = M.blank((246, 244, 236, 255))
    for i in range(24):
        px[23][i] = (220, 216, 204, 255)
        px[i][23] = (220, 216, 204, 255)
    return M.save("mplace-blank", px)


def bucket_sprite(c):
    px = M.blank()
    M.rect(px, 5, 9, 18, 20, M.P["steel"])
    M.rect(px, 5, 9, 7, 20, M.P["steel_hi"])
    M.rect(px, 16, 9, 18, 20, M.P["steel_dark"])
    M.ellipse(px, 12, 9, 7.5, 2.6, rgba(RGB[c]))
    M.rect(px, 9, 10, 10, 14, rgba(RGB[c]))  # drip
    M.ellipse(px, 9.5, 14.5, 1.4, 1.4, rgba(RGB[c]))
    for x in range(5, 19):  # handle
        t = (x - 11.5) / 6.5
        M.put(px, x, round(5 + 3 * t * t), M.P["steel_dark"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22, rx=8)
    return M.save(f"mplace-bucket-{c}", px)


def brush_blob(px, c):
    for y in range(24):
        for x in range(24):
            dx, dy = x + 0.5 - 19.5, y + 0.5 - 19.5
            if dx * dx + dy * dy <= 13:
                px[y][x] = M.P["outline"]
            if dx * dx + dy * dy <= 7:
                px[y][x] = rgba(RGB[c])


def avatar_sprites():
    base = M.bufo("plain")
    out = list(base)
    for c in COLOURS:
        for rel, d in zip(base, M.FACINGS):
            out.append(K.overlay(rel, f"mplace-bufo-{c}-{d}", lambda px, c=c: brush_blob(px, c)))
    return out


def troll_sprite(col):
    px = K.load(M.bufo_frames("plain")[0])
    body = {"black": (120, 90, 140), "green": (150, 110, 70)}[col]
    for y in range(24):
        for x in range(24):
            r, g, b, a = px[y][x]
            if a and g > r + 25 and g > b + 25:  # recolour the green bufo skin
                k = (r + g + b) / (3 * 150)
                px[y][x] = tuple(min(255, int(v * k)) for v in body) + (a,)
    for x, y in ((5, 6), (6, 5), (17, 5), (18, 6)):  # angry brows
        px[y][x] = M.P["black"]
    brush_blob(px, col if col != "black" else "black")
    return M.save(f"mplace-troll-{col}", px)


# ------------------------------------------------------------------------------ GDY
def actions():
    pix = ", ".join(PIXELS)
    walk_dst = f"[_empty, blank, {pix}]"
    face = "{set: [fc, meta.facing]}, {set: [ft, fc]}, {add: [ft, tb]}, {set_tile: ft}"
    # a pixel's own commands when painted with colour `src.col` (paint index 0 = unpainted ghost)
    repaint = ("{if: {Conditions: {eq: [paint, tgt]}, OnTrue: [{incr: wrong}]}}, {set: [paint, src.col]}, "
               "{if: {Conditions: {eq: [paint, tgt]}, OnTrue: [{decr: wrong}]}}, {set_tile: paint}")
    out = [C.face_action("bufo"), "  # Absolute 4-way walk. Buckets give their colour; jars give mayo.\n",
           "  - Name: move\n", C.MOVE_INPUTS, "    Behaviours:\n",
           f"      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {face}, {{mov: _dest}}]}}\n"
           f"        Dst: {{Object: {walk_dst}}}\n",
           f"      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {face}, {{mov: _dest}}, {{add: [mayo, {JAR}]}}, "
           f"{{reward: 1}}]}}\n        Dst: {{Object: jar, Commands: [{{remove: true}}]}}\n",
           f"      - Src: {{Object: bufo, Commands: [{{set: [col, dst.col]}}, {{set: [tb, dst.tb]}}, "
           f"{{set_tile: meta.facing}}, {face}, {{mov: _dest}}]}}\n        Dst: {{Object: [{', '.join(BUCKETS)}]}}\n",
           f"      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {face}]}}\n"
           f"        Dst: {{Object: [wall, rack, logo, plant, {', '.join(TROLL_NAMES)}]}}\n"]
    out.append("  # Paint the pixel in front of you with your colour (1 mayo), or splat a troll with mayo (ban).\n")
    out.append("  - Name: paint\n" + C.MOVE_INPUTS + "    Behaviours:\n")
    out.append(f"      - Src:\n          Object: bufo\n          Preconditions: [{{gte: [mayo, 1]}}]\n"
               f"          Commands: [{{set_tile: meta.facing}}, {face}, {{decr: mayo}}, {{decr: supply}}, {{reward: 2}}]\n"
               f"        Dst: {{Object: [{', '.join(TROLL_NAMES)}], Commands: [{{remove: true}}]}}\n")
    out.append(f"      - Src:\n          Object: bufo\n          Preconditions: [{{gte: [mayo, 1]}}, {{gte: [src.col, 1]}}, "
               f"{{neq: [src.col, dst.paint]}}]\n"
               f"          Commands: [{{set_tile: meta.facing}}, {face}, {{decr: mayo}}, {{decr: supply}}, "
               f"{{if: {{Conditions: {{eq: [dst.paint, dst.tgt]}}, OnTrue: [{{reward: 1}}]}}}}]\n"
               f"        Dst: {{Object: [{pix}], Commands: [{repaint}]}}\n")
    tinputs = "\n".join(f"        {k}: {{VectorToDest: [{v[0]}, {v[1]}], MetaData: {{d: {k}}}}}" for k, v in DIRS.items())
    rev = ("{set: [nd, 0]}, {if: {Conditions: {eq: [dir, 1]}, OnTrue: [{set: [nd, 3]}]}}, "
           "{if: {Conditions: {eq: [dir, 2]}, OnTrue: [{set: [nd, 4]}]}}, "
           "{if: {Conditions: {eq: [dir, 3]}, OnTrue: [{set: [nd, 1]}]}}, "
           "{if: {Conditions: {eq: [dir, 4]}, OnTrue: [{set: [nd, 2]}]}}, {set: [dir, nd]}")
    again = f"{{exec: {{Action: troll, ActionId: dir, Delay: {TROLL_PERIOD}}}}}"
    trolls = ", ".join(TROLL_NAMES)
    out.append("  # Trolls walk a straight lane every few ticks, turn back at anything solid (you included) and\n"
               "  # paint every canvas pixel they step on in their own colour.\n")
    out.append(f"  - Name: troll\n    InputMapping:\n      Internal: true\n      Inputs:\n{tinputs}\n    Behaviours:\n")
    out.append(f"      - Src: {{Object: [{trolls}], Commands: [{{set: [dir, meta.d]}}, {{mov: _dest}}, {again}]}}\n"
               f"        Dst: {{Object: [_empty, blank]}}\n")
    out.append(f"      - Src: {{Object: [{trolls}], Commands: [{{set: [dir, meta.d]}}, {{mov: _dest}}, {again}]}}\n"
               f"        Dst: {{Object: [{pix}], Commands: [{repaint}]}}\n")
    out.append(f"      - Src: {{Object: [{trolls}], Commands: [{{set: [dir, meta.d]}}, {rev}, {again}]}}\n"
               f"        Dst: {{Object: [_boundary, wall, rack, logo, plant, jar, bufo, {', '.join(BUCKETS)}, {trolls}]}}\n")
    out.append(f"""  - Name: count_wrong
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: [{pix}], Commands: [{{incr: wrong}}]}}
        Dst: {{Object: [{pix}]}}
  - Name: count_jar
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: jar, Commands: [{{add: [supply, {JAR}]}}]}}
        Dst: {{Object: jar}}
""")
    return "\nActions:\n" + "".join(out)


def ivars(**kw):
    return "    Variables:\n" + "".join(f"      - Name: {k}\n        InitialValue: {v}\n" for k, v in kw.items())


def objects():
    acts, decor = K.decor_objects()
    jar_anim, jar_init = M.animate("jar", 2, 4)
    jar_extra = "    InitialActions:\n      - Action: count_jar\n        ActionId: 1\n" + jar_init.split("\n", 1)[1]
    out = [K.wall_object(), decor,
           C.obj("blank", "e", 0, C.img(blank_canvas_sprite()), "[0.95, 0.95, 0.9]"),
           C.obj("jar", "j", 1, C.imgs(M.mayo_jar_frames()), "[1.0, 1.0, 0.8]", "triangle", 0.7, jar_extra, tiles=2)]
    count = "    InitialActions:\n      - Action: count_wrong\n        ActionId: 1\n"
    for t in COLOURS:
        rgb = "[%.2f, %.2f, %.2f]" % tuple(v / 255 for v in RGB[t])
        sprites = [ghost_sprite(t)] + [painted_sprite(t, p) for p in COLOURS]
        out.append(C.obj(f"px_{t}", GHOST_CH[t], 0, C.imgs(sprites), rgb, "square", 0.6,
                         ivars(tgt=CI[t], paint=0) + count, tiles=len(sprites)))
        out.append(C.obj(f"bucket_{t}", BUCKET_CH[t], 1, C.img(bucket_sprite(t)), rgb, "triangle", 0.6,
                         ivars(col=CI[t], tb=4 * CI[t])))
    for ch in "tx":
        col, _ = TROLLS[ch]
        spr = C.img(troll_sprite(col))
        for c2, (col2, d) in TROLLS.items():
            if col2 != col:
                continue
            init = ivars(dir=d, nd=0, col=CI[col]) + \
                f"    InitialActions:\n      - Action: troll\n        ActionId: {d}\n        Delay: {TROLL_PERIOD}\n"
            out.append(C.obj(troll_name(c2), c2, 1, spr, "[0.5, 0.2, 0.6]", "triangle", 0.8, init))
    out.append(C.obj("bufo", "A", 2, C.imgs(avatar_sprites()), "[0.3, 0.8, 0.3]", "triangle", 0.8,
                     ivars(col=0, tb=0, fc=2, ft=2) + C.FACE_INITIAL_ACTION, tiles=4 * (len(COLOURS) + 1)))
    return acts + jar_anim, "\nObjects:\n" + "".join(out)


# ------------------------------------------------------------------------------ main
def validate(name, rows, want_trolls):
    C.check_rect(name, rows)
    assert sum(r.count("A") for r in rows) == 1, f"{name}: one avatar"
    sim = Sim(rows)
    colours = {t for t, _ in sim.cells.values()}
    have = set(sim.buckets.values())
    assert colours <= have, f"{name}: missing buckets for {colours - have}"
    assert bool(sim.trolls) == bool(want_trolls), f"{name}: trolls"
    for tr in sim.trolls:
        assert tr["pos"] not in sim.cells, f"{name}: troll starts on canvas"
    res, end, run = plan(rows)
    assert res == "Win", f"{name}: planner {res} after {len(run)} steps (wrong={end.wrong} supply={end.supply})"
    return sim, end, run


def diff(gdy, runs):
    """Compare Sim against the engine after every step of each planned run."""
    here = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory() as td:
        gp, jp, op = Path(td, "g.yaml"), Path(td, "j.json"), Path(td, "o.json")
        gp.write_text(gdy)
        keep = ["bufo", "jar"] + TROLL_NAMES
        jp.write_text(json.dumps([{"level": i, "actions": run, "keep": keep} for i, run in enumerate(runs)]))
        subprocess.run(["node", str(here / "office_dump.mjs"), str(gp), str(jp), str(op)], check=True,
                       stdout=subprocess.DEVNULL)
        dumps = json.loads(op.read_text())
    bad = 0
    for i, (run, states) in enumerate(zip(runs, dumps)):
        sim = Sim(LEVELS[i][2]())
        for k, st in enumerate(states):
            if k:
                res = sim.step(*run[k - 1])
                if (res or None) != st["term"]:
                    print(f"  L{i} step {k}: term sim={res} engine={st['term']}", file=sys.stderr)
                    bad += 1
                    break
            av = next(o for o in st["objs"] if o[0] == "bufo")
            trolls = sorted((o[1], o[2]) for o in st["objs"] if o[0].startswith("troll"))
            g = st["globals"]
            mine = (tuple(sim.pos), sim.mayo, sim.supply, sim.wrong, sorted(t["pos"] for t in sim.trolls if t["alive"]),
                    len(sim.jars))
            eng = ((av[1], av[2]), g["mayo"], g["supply"], g["wrong"], trolls, sum(o[0] == "jar" for o in st["objs"]))
            if mine != eng:
                print(f"  L{i} step {k} {run[k - 1] if k else ''}: sim={mine} engine={eng}", file=sys.stderr)
                bad += 1
                break
        else:
            print(f"  L{i}: sim == engine over {len(states)} states", file=sys.stderr)
    return bad


def main():
    header = C.header("m-place", """
        m/place: one mayo = one pixel. Walk onto a paint bucket to pick up its colour, then paint the
        pixel in front of you (Space) to match the ghost image on the canvas. Each pixel costs 1 mayo;
        mayo jars refill you. Troll bufos walk straight lanes and scribble over every pixel they step on:
        throw mayo at one (paint it) to ban it. Win when the canvas matches the ghost image; lose when
        your mayo (held + in jars) can't cover the pixels still wrong.
        """.replace("        ", ""), "bufo", ["eq: [wrong, 0]"], ["lt: [supply, wrong]"],
        background=K.floor_bg(), variables="    - Name: mayo\n      InitialValue: 0\n"
                                            "    - Name: supply\n      InitialValue: 0\n"
                                            "    - Name: wrong\n      InitialValue: 0\n")
    parts = [header]
    runs = []
    for i, (name, blurb, build, trolls) in enumerate(LEVELS):
        rows = build()
        sim0, end, run = validate(name, rows, trolls)
        runs.append(run)
        parts.append(C.level_block(name, blurb, rows))
        print(f"{i}. {name:20s} {len(rows[0])}x{len(rows)} pixels={len(sim0.cells)} jars={len(sim0.jars)} "
              f"(mayo {JAR * len(sim0.jars)}) trolls={len(sim0.trolls)} -> Win in {len(run)} steps, "
              f"{end.paints} paints, mayo left {end.supply}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    acts, objs = objects()
    gdy = "".join(parts) + actions() + acts + objs
    if "--diff" in sys.argv:
        sys.exit(1 if diff(gdy, runs) else 0)
    K.write_solutions2(GID, runs)
    C.emit(GID, gdy, {
        "name": "m/place",
        "tagline": "1 mayo = 1 pixel. Paint the office canvas, ban the trolls.",
        "description": "The office pixel canvas, bufo edition: pick a colour at a paint bucket and paint the "
                       "ghost image pixel by pixel, one mayo each. Mayo jars refill you, and troll bufos "
                       "scribble over your art until you splat them with mayo.",
        "tags": ["meme", "puzzle", "planning"],
        "difficulty": "medium",
        "original": "new (Mistral office meme)",
        "accent": "#ff8205",
        "icon": "🎨",
        "controls": {"move": "move", "extra": [{"key": " ", "label": "Paint / splat", "action": "paint",
                                                "directional": True}]},
        "hud": [{"label": "Mayo", "kind": "global", "variable": "mayo", "tone": "amber"},
                {"label": "Pixels left", "kind": "global", "variable": "wrong", "tone": "rose"},
                {"label": "Mayo + jars", "kind": "global", "variable": "supply", "tone": "sky"},
                {"label": "Jars", "kind": "remaining", "object": "jar", "tone": "lime"}],
        "win_text": "The canvas matches! 1 mayo = 1 pixel, well spent.",
        "lose_text": "Not enough mayo left for the pixels still wrong.",
    })


if __name__ == "__main__":
    main()
