#!/usr/bin/env python3
"""Rack Packer: sokoban in the datacenter. A new "meme" game, not a port.

Bufo pushes GPU cards into empty rack slots. Rules (the GDY below; `Sim` mirrors them exactly and is
checked against the engine with `--diff`):
  * Walk into a card to push it (one card at a time; cards don't push cards). A card pushed into an
    empty rack SLOT installs: the slot becomes a busy rack.
  * HOT cards (red) must be cooled first: a hot card that slides onto a COOLANT puddle becomes a cool
    card. A hot card pushed into a slot sets the rack on fire = you lose.
  * Mistral-logo CONVEYOR BELTS carry cards one cell per tick in the arrow's direction, from belt to
    belt, until the card leaves the belts (onto floor, coolant or into a slot) or hits something - then it
    stops where it is. Bufo's sticky toad feet don't slip: belts never move bufo, who may walk on them.
    A card riding a belt can't be pushed (wait for it to stop; `nap` = Z waits a tick).
  * Win: every slot holds a GPU.
Determinism: rides are exec timers (Delay 1) only. Two cards riding at the same time can interact in
engine order; the solver never does that (it waits for every ride to finish before the next push).

Layout characters: W wall  R server rack (solid)  A bufo  g cool GPU card  h hot GPU card  o empty slot
  k coolant puddle (floor)  < > n v conveyor belt (west east north south)  x cable (floor)  . floor
Validation per level: rectangular, one bufo, #cards == #slots; an A* search over push events (rides
simulated exactly) finds a solution, which is written to apps/web/src/games/solutions/rackpacker.json.
Levels with belts / coolant also prove the mechanic matters (ablation: unsolvable without it).
"""
from __future__ import annotations

import heapq
import itertools
import json
import random
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

GID = "rackpacker"
HERE = Path(__file__).resolve().parent
SOLUTIONS = C.ROOT / "studio/griddly-demo/apps/web/src/games/solutions" / f"{GID}.json"
DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
BELT_CHAR = {"<": 1, "n": 2, ">": 3, "v": 4}
BELT_NAME = {1: "belt_w", 2: "belt_n", 3: "belt_e", 4: "belt_s"}
BELTS = list(BELT_NAME.values())
FLOOR = ["cable", "coolant"]           # Z=0, walkable for bufo and cards
STATIC = ["wall", "rack", "installed", "slotfire"]
CARDS = ["gpu", "hotgpu"]
ALL = FLOOR + BELTS + STATIC + CARDS + ["slot", "bufo"]


def lst(names):
    return "[" + ", ".join(names) + "]"


# ----------------------------------------------------------------------------------------------
# Art (original; memekit helpers)
# ----------------------------------------------------------------------------------------------
def art():
    P = M.PALETTE
    a = {}
    # conveyor belt: dark belt with rollers, the Mistral M on top, the yellow arrow at the exit edge;
    # 2 frames (the rollers' stripes shift)
    load = lambda rel: C.load_png(C.IMAGES_DIR / rel)[2]
    logo = load(M.mistral_logo())
    for d, dn in ((1, "left"), (2, "up"), (3, "right"), (4, "down")):
        frames = []
        for f in range(2):
            px = M.blank((46, 46, 54, 255))
            horiz = d in (1, 3)
            for y in range(24):
                for x in range(24):
                    u = x if horiz else y
                    sgn = 1 if d in (3, 4) else -1
                    if (u - sgn * f * 2) % 4 == 0:
                        px[y][x] = (70, 70, 82, 255)
            M.rect(px, 0, 0, 23, 0, (26, 26, 32, 255))
            M.rect(px, 0, 23, 23, 23, (26, 26, 32, 255))
            small = M.blank()
            for y in range(24):
                for x in range(24):
                    sx, sy = int((x - 3) / 0.75), int((y - 3) / 0.75)
                    if 0 <= sx < 24 and 0 <= sy < 24:
                        small[y][x] = logo[sy][sx]
            M.paste(px, small)
            px = M.add_arrow(px, dn)
            frames.append(M.save(f"rackpacker-belt-{dn}-{f}", px))
        a[BELT_NAME[d]] = frames
    # coolant puddle (floor decal)
    px = M.blank()
    M.ellipse(px, 12, 13, 10, 8, P["coolant"], P["coolant_hi"], (30, 140, 180, 255))
    for x, y in ((8, 10), (9, 10), (15, 15), (16, 15)):
        M.put(px, x, y, P["white"])
    a["coolant"] = M.save("rackpacker-coolant", px)
    # rack slot on fire
    rack = load(M.server_rack("empty"))
    fl = load(M.gpu_fire_frames()[1])
    px = M.copy(rack)
    M.paste(px, fl)
    a["slotfire"] = M.save("rackpacker-slotfire", px)
    a["gpu"] = M.gpu("cool")
    a["hotgpu"] = M.gpu("hot")
    a["slot"] = M.server_rack("empty")
    a["installed"] = M.rack_frames()
    a["rack"] = M.server_rack("idle")
    a["cable"] = M.cable()
    a["bufo"] = M.bufo("gpu")
    a["floor"] = M.datacenter_floor()
    M.datacenter_wall16()
    return a


# ----------------------------------------------------------------------------------------------
# Levels
# ----------------------------------------------------------------------------------------------
LEVELS = [
    ("Day One", "Push the three GPU cards into the empty rack slots.", {}, [
        "WWWWWWWWWWWWWWWWW",
        "W.......W.......W",
        "W..g....W..RoR..W",
        "W...........x...W",
        "W..A..g.....x...W",
        "W......WW...xx..W",
        "W..g...W.....x..W",
        "W......W....RoRRW",
        "WRoRWWWWWWWWWWWWW",
        "WWWWWWWWWWWWWWWWW",
    ]),
    ("Hot Swap", "Hot cards set racks on fire. Slide them over a coolant puddle first.", {"ablate": "coolant"}, [
        "WWWWWWWWWWWWWWWWWW",
        "W........W.......W",
        "W..h.....W...h...W",
        "W.....k......k...W",
        "W..A.........g...W",
        "W....WWW.........W",
        "W....W...........W",
        "WRRoRRRRRRoRRRoRRW",
        "WWWWWWWWWWWWWWWWWW",
    ]),
    ("Conveyor", "Mistral belts carry cards one cell per tick - even round corners you can't push them.",
     {"ablate": "belts"}, [
        "WWWWWWWWWWWWWWWWWWW",
        "WWWWWWWWWWWWWWWWWoW",
        "W..g....>>>>>>>>>nW",
        "W.......nWWWWWWWWWW",
        "W..A....n.........W",
        "W.......n.........W",
        "W..g....n.........W",
        "W.................W",
        "WRRRoRRRRRRRRRRRRRW",
        "WWWWWWWWWWWWWWWWWWW",
    ]),
    ("Supply Chain", "A hot card rides the belt over a coolant puddle. Mind where it stops.", {"ablate": "coolant"}, [
        "WWWWWWWWWWWWWWWWWWWW",
        "W........WWWWWWWWRoW",
        "W..g.....W..>>k>>v.W",
        "W........W..n.WW.v.W",
        "W..A..h.....n.WW.v.W",
        "W...........n....v.W",
        "W.....k..W..h......W",
        "W........W.........W",
        "WRRoRRRRRWRRRRRoRRRW",
        "WWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Rack Packer", "Everything at once: belts, coolant, hot cards and a long rack row.", {"ablate": "belts"}, [
        "WWWWWWWWWWWWWWWWWWWWWW",
        "W.........WWWWWWWWWWoW",
        "W..h......W..>>>>k>>nW",
        "W.....k......n.WWWWWWW",
        "W..A.........n.......W",
        "W..g......W..n...h...W",
        "W.........W..n.......W",
        "W..g......W..........W",
        "WRRoRRRRoRWRRRRRoRRRRW",
        "WWWWWWWWWWWWWWWWWWWWWW",
    ]),
]


# ----------------------------------------------------------------------------------------------
# Simulator
# ----------------------------------------------------------------------------------------------
class Level:
    def __init__(self, rows):
        self.rows = rows
        self.h, self.w = len(rows), len(rows[0])
        self.belt = {}       # cell -> dir
        self.coolant = set()
        self.wall = set()    # walls + racks
        self.slots = set()
        cool, hot = set(), set()
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                c = (x, y)
                if ch in "WR":
                    self.wall.add(c)
                elif ch in BELT_CHAR:
                    self.belt[c] = BELT_CHAR[ch]
                elif ch == "k":
                    self.coolant.add(c)
                elif ch == "o":
                    self.slots.add(c)
                elif ch == "g":
                    cool.add(c)
                elif ch == "h":
                    hot.add(c)
                elif ch == "A":
                    self.start = c
        self.state0 = (self.start, frozenset(cool), frozenset(hot), frozenset(self.slots))

    def walk(self, c):
        return 0 <= c[0] < self.w and 0 <= c[1] < self.h and c not in self.wall and c not in self.slots


def add(p, d):
    return p[0] + d[0], p[1] + d[1]


def move_card(lv: Level, st, card, d):
    """Card at `card` moves one cell in dir d (push or ride). Returns (new_state, outcome) where outcome
    is 'stop' / 'ride' (landed on a belt) / 'blocked' / 'install' / 'fire'."""
    me, cool, hot, slots = st
    n = add(card, DIRS[d])
    is_hot = card in hot
    if n in lv.wall or n in cool or n in hot or n == me or not (0 <= n[0] < lv.w and 0 <= n[1] < lv.h):
        return st, "blocked"
    if n in lv.slots:
        if n not in slots:
            return st, "blocked"  # installed rack
        if is_hot:
            return (me, cool, hot - {card}, slots - {n}), "fire"
        return (me, cool - {card}, hot, slots - {n}), "install"
    if is_hot:
        if n in lv.coolant:
            hot2, cool2 = hot - {card}, cool | {n}
        else:
            hot2, cool2 = (hot - {card}) | {n}, cool
    else:
        hot2, cool2 = hot, (cool - {card}) | {n}
    return (me, frozenset(cool2), frozenset(hot2), slots), ("ride" if n in lv.belt else "stop")


def push(lv: Level, st, d, use_belts=True):
    """Bufo at st[0] pushes in dir d. Returns (state, ticks_waited, outcome) with the ride simulated to the
    end (bufo naps meanwhile), or None if nothing is pushed."""
    me = st[0]
    card = add(me, DIRS[d])
    if card not in st[1] and card not in st[2]:
        return None
    ns, out = move_card(lv, st, card, d)
    if out == "blocked":
        return None
    ns = (card,) + ns[1:]
    pos = add(card, DIRS[d])
    ticks = 0
    while out == "ride" and use_belts:
        bd = lv.belt[pos]
        ticks += 1
        ns, out = move_card(lv, ns, pos, bd)
        if out in ("ride", "stop", "install", "fire"):
            pos = add(pos, DIRS[bd])
        if out == "blocked":
            out = "stop"
    return ns, ticks, out


# ----------------------------------------------------------------------------------------------
# Solver: A* over push events
# ----------------------------------------------------------------------------------------------
def region(lv: Level, st):
    me, cool, hot, _ = st
    par = {me: None}
    q = [me]
    for c in q:
        for m, d in DIRS.items():
            n = add(c, d)
            if n not in par and lv.walk(n) and n not in cool and n not in hot:
                par[n] = (c, m)
                q.append(n)
    return par


def path_to(par, cell):
    out = []
    while par[cell] is not None:
        cell, m = par[cell]
        out.append(m)
    return out[::-1]


def dead_cells(lv: Level):
    """Floor cells (not belts) where a card can never move again: two orthogonal static walls (corner)."""
    dead = set()
    for y in range(lv.h):
        for x in range(lv.w):
            c = (x, y)
            if c in lv.wall or c in lv.slots or c in lv.belt:
                continue
            blk = [add(c, d) in lv.wall for d in DIRS.values()]  # W N E S
            if (blk[0] or blk[2]) and (blk[1] or blk[3]):
                dead.add(c)
    return dead


def heuristic(lv: Level, st, dist):
    _, cool, hot, slots = st
    h = 0
    for c in cool:
        h += min((abs(c[0] - s[0]) + abs(c[1] - s[1]) for s in slots), default=0)
    for c in hot:
        h += min((abs(c[0] - k[0]) + abs(c[1] - k[1]) for k in lv.coolant), default=99) + 2
    return h


def solve(lv: Level, use_belts=True, use_coolant=True, max_nodes=300_000, weight=2.0):
    dead = dead_cells(lv)
    coolant = lv.coolant
    if not use_coolant:
        lv.coolant = set()
    try:
        s0 = lv.state0
        par0 = region(lv, s0)
        key = lambda st, par: (min(par),) + st[1:]
        k0 = key(s0, par0)
        best = {k0: 0}
        link = {k0: None}
        cnt = itertools.count()
        heap = [(weight * heuristic(lv, s0, None), 0, next(cnt), s0, k0, par0)]
        nodes = 0
        while heap:
            _, g, _, st, k, par = heapq.heappop(heap)
            if best.get(k, 1 << 30) < g:
                continue
            nodes += 1
            if nodes > max_nodes:
                return None, nodes
            for cell in list(par):
                for m, d in DIRS.items():
                    t = add(cell, d)
                    if t not in st[1] and t not in st[2]:
                        continue
                    res = push(lv, (cell,) + st[1:], m, use_belts)
                    if res is None:
                        continue
                    ns, ticks, out = res
                    if out == "fire":
                        continue
                    moved = ns[1] | ns[2]
                    if any(c in dead for c in moved if c not in st[1] and c not in st[2]):
                        continue
                    # the first ride runs in the push step itself (Delay 1 = this step's tick update)
                    moves = [("move", x) for x in path_to(par, cell)] + [("move", m)] + [("nap", 1)] * max(0, ticks - 1)
                    if not ns[3]:
                        seq = [moves]
                        kk = k
                        while link[kk] is not None:
                            pk, mv = link[kk]
                            seq.append(mv)
                            kk = pk
                        return [x for part in reversed(seq) for x in part], nodes
                    npar = region(lv, ns)
                    nk = key(ns, npar)
                    if best.get(nk, 1 << 30) <= g + 1:
                        continue
                    best[nk] = g + 1
                    link[nk] = (k, moves)
                    heapq.heappush(heap, (g + 1 + weight * heuristic(lv, ns, None), g + 1, next(cnt), ns, nk, npar))
        return None, nodes
    finally:
        lv.coolant = coolant


# ----------------------------------------------------------------------------------------------
# Exact per-tick simulator (for --diff and to check solutions): mirrors the engine step by step
# ----------------------------------------------------------------------------------------------
class Sim:
    def __init__(self, lv: Level):
        self.lv = lv
        self.st = lv.state0
        self.t = 0
        self.riding = {}  # card cell -> (due tick, dir)
        self.result = None

    def step(self, action, d):
        lv = self.lv
        if self.result:
            return
        me, cool, hot, slots = self.st
        if action == "move" and d in DIRS:
            n = add(me, DIRS[d])
            if (n in cool or n in hot) and n not in self.riding:
                ns, out = move_card(lv, self.st, n, d)
                if out != "blocked":
                    self.st = (n,) + ns[1:]
                    self._after(add(n, DIRS[d]), out, self.t + 1)
            elif lv.walk(n) and n not in cool and n not in hot:
                self.st = (n,) + self.st[1:]
        self.t += 1
        due = sorted((c, v) for c, v in self.riding.items() if v[0] <= self.t)
        for c, (_, bd) in due:
            del self.riding[c]
            ns, out = move_card(lv, self.st, c, bd)
            if out == "blocked":
                continue
            self.st = ns
            self._after(add(c, DIRS[bd]), out, self.t + 1)
        if not self.st[3]:
            self.result = "Win"
        if self.fire:
            self.result = "Lose"

    fire = False

    def _after(self, pos, out, due):
        if out == "ride":
            self.riding[pos] = (due, self.lv.belt[pos])
        elif out == "fire":
            self.fire = True


# ----------------------------------------------------------------------------------------------
# GDY
# ----------------------------------------------------------------------------------------------
def card_behaviours(card):
    """Behaviours of a card doing `move` (pushed) or `ride` (carried by a belt)."""
    floor = lst(["_empty"] + FLOOR) if card == "gpu" else lst(["_empty", "cable"])
    out = [f"      - Src: {{Object: {card}, Commands: [{{mov: _dest}}, {{set: [riding, 0]}}]}}\n"
           f"        Dst: {{Object: {floor}}}\n"]
    if card == "hotgpu":
        out.append("      - Src: {Object: hotgpu, Commands: [{mov: _dest}, {change_to: gpu}]}\n"
                   "        Dst: {Object: coolant}\n")
    for d, name in BELT_NAME.items():
        out.append(f"      - Src: {{Object: {card}, Commands: [{{mov: _dest}}, {{set: [riding, 1]}}, "
                   f"{{exec: {{Action: ride, ActionId: {d}, Delay: 1}}}}]}}\n        Dst: {{Object: {name}}}\n")
    if card == "gpu":
        out.append("      - Src: {Object: gpu, Commands: [{remove: true}]}\n"
                   "        Dst: {Object: slot, Commands: [{decr: open}, {change_to: installed}]}\n")
    else:
        out.append("      - Src: {Object: hotgpu, Commands: [{remove: true}]}\n"
                   "        Dst: {Object: slot, Commands: [{incr: melted}, {change_to: slotfire}]}\n")
    blocked = [n for n in ALL if n not in FLOOR + BELTS + ["slot"]]
    out.append(f"      - Src: {{Object: {card}, Commands: [{{set: [riding, 0]}}]}}\n"
               f"        Dst: {{Object: {lst(blocked)}}}\n")
    return "".join(out)


def actions(a):
    face = "{set_tile: meta.facing}"
    walk = lst(["_empty"] + FLOOR + BELTS)
    belt_anims = ""
    for name in BELTS:
        an, _ = M.animate(name, 2, 2, over=("gpu", "hotgpu", "bufo"))
        belt_anims += an
    rack_anim, _ = M.animate("installed", 2, 4)
    return f'''
Actions:
{C.face_action("bufo")}
  # Bufo walks (floor, coolant, belts: sticky toad feet) and pushes a card that isn't riding a belt.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: bufo, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {walk}}}
      - Src:
          Object: bufo
          Preconditions: [{{eq: [dst.riding, 0]}}]
          Commands: [{face}, {{mov: _dest}}]
        Dst: {{Object: {lst(CARDS)}, Commands: [{{cascade: _dest}}]}}
      - Src:
          Object: bufo
          Preconditions: [{{eq: [dst.riding, 1]}}]
          Commands: [{face}]
        Dst: {{Object: {lst(CARDS)}}}
      - Src: {{Object: bufo, Commands: [{face}]}}
        Dst: {{Object: {lst(STATIC + ["slot"])}}}
{card_behaviours("gpu")}{card_behaviours("hotgpu")}
  # A belt carries the card on it one cell per tick (the card schedules its next ride when it lands on a
  # belt; it stops when it leaves the belts or bumps into something).
  - Name: ride
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
{card_behaviours("gpu")}{card_behaviours("hotgpu")}
  # Count the empty slots at reset (win: open == 0).
  - Name: count_slot
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: slot, Commands: [{{incr: open}}]}}
        Dst: {{Object: slot}}

  # Wait one tick (let the belts do the work).
  - Name: nap
    InputMapping:
      Inputs:
        1: {{Description: Nap, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: bufo, Commands: [{{nop: true}}]}}
        Dst: {{Object: bufo}}
{belt_anims}{rack_anim}'''


def objects(a):
    card_vars = "    Variables:\n      - Name: riding\n        InitialValue: 0\n"
    objs = [
        C.obj("wall", "W", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("rack", "R", 1, C.img(a["rack"]), "[0.2, 0.2, 0.25]", "square", 0.95),
        C.obj("cable", "x", 0, C.img(a["cable"]), "[0.2, 0.4, 0.9]", "square", 0.4),
        C.obj("coolant", "k", 0, C.img(a["coolant"]), "[0.2, 0.7, 0.9]", "circle", 0.8),
    ]
    for d, name in BELT_NAME.items():
        ch = {1: "<", 2: "n", 3: ">", 4: "v"}[d]
        _, ini = M.animate(name, 2, 2, over=("gpu", "hotgpu", "bufo"))
        objs.append(C.obj(name, ch, 0, C.imgs(a[name]), "[1.0, 0.55, 0.0]", "triangle", 0.6, ini, tiles=2))
    _, rack_ini = M.animate("installed", 2, 4)
    objs += [
        C.obj("slot", "o", 1, C.img(a["slot"]), "[0.1, 0.1, 0.12]", "square", 0.95,
              "    InitialActions:\n      - Action: count_slot\n        ActionId: 1\n"),
        C.obj("installed", None, 1, C.imgs(a["installed"]), "[0.2, 0.8, 0.3]", "square", 0.95, rack_ini, tiles=2),
        C.obj("slotfire", None, 1, C.img(a["slotfire"]), "[1.0, 0.2, 0.0]", "square", 0.95),
        C.obj("gpu", "g", 1, C.img(a["gpu"]), "[0.2, 0.8, 0.4]", "square", 0.7, card_vars),
        C.obj("hotgpu", "h", 1, C.img(a["hotgpu"]), "[1.0, 0.3, 0.1]", "square", 0.7, card_vars),
        C.obj("bufo", "A", 1, C.imgs(a["bufo"]), "[0.4, 0.7, 0.25]", "triangle", 0.9, C.FACE_INITIAL_ACTION, tiles=4),
    ]
    return "\nObjects:\n" + "".join(objs)


def header(a):
    variables = "".join(f"    - Name: {n}\n      InitialValue: 0\n" for n in ("open", "melted"))
    return C.header("Rack Packer", """
        Sokoban in the datacenter. Bufo pushes GPU cards into empty rack slots. Hot cards must slide over
        a coolant puddle first (a hot card in a rack sets it on fire). Mistral-logo conveyor belts carry
        cards one cell per tick until they leave the belt or bump into something. Fill every slot.
        """.replace("        ", ""), "bufo", ["eq: [open, 0]"], ["gt: [melted, 0]"],
        background=a["floor"], variables=variables)


# ----------------------------------------------------------------------------------------------
# main / diff
# ----------------------------------------------------------------------------------------------
def validate(name, rows):
    C.check_rect(name, rows)
    assert sum(r.count("A") for r in rows) == 1, f"{name}: need one bufo"
    for r in rows:
        assert set(r) <= set("WRAghokx.") | set(BELT_CHAR), f"{name}: unknown chars in {r}"
    nc = sum(r.count("g") + r.count("h") for r in rows)
    ns = sum(r.count("o") for r in rows)
    assert nc == ns and nc > 0, f"{name}: {nc} cards vs {ns} slots"


def check_plan(lv, plan):
    s = Sim(lv)
    for i, (a, d) in enumerate(plan):
        s.step(a, d)
        if s.result:
            return s.result == "Win" and i == len(plan) - 1
    return False


def main():
    a = art()
    parts = [header(a)]
    sols = []
    for i, (name, blurb, hints, rows) in enumerate(LEVELS):
        validate(name, rows)
        lv = Level(rows)
        parts.append(C.level_block(name, blurb, rows))
        t0 = time.time()
        plan, nodes = solve(lv)
        assert plan is not None, f"{name}: no solution found ({nodes} nodes)"
        assert check_plan(lv, plan), f"{name}: plan does not win in the tick simulator"
        abl = ""
        if hints.get("ablate") == "belts":
            # without belts a card only moves by being pushed: a slot whose every approach needs bufo to stand
            # in a wall can never be filled.
            cut = [sl for sl in lv.slots if not any(add(sl, (-dx, -dy)) not in lv.wall and
                                                     add(sl, (-2 * dx, -2 * dy)) not in lv.wall
                                                     for dx, dy in DIRS.values())]
            assert cut, f"{name}: every slot can be filled by pushing alone"
            abl = f"  belts needed: slot(s) {sorted(cut)} can't be reached by a push"
        if hints.get("ablate") == "coolant" or any(ch == "h" for r in rows for ch in r):
            assert any(ch == "h" for r in rows for ch in r) and lv.coolant, f"{name}: coolant hint without hot cards"
            abl += "  coolant needed: hot cards only cool on coolant"
        sols.append({"seed": 1, "actions": [list(x) for x in plan]})
        pushes = sum(1 for k, (x, _) in enumerate(plan) if x == "move")
        print(f"  L{i} {name:14s} {len(rows[0])}x{len(rows)} cards={len(lv.slots)} solution {len(plan)} steps "
              f"({nodes} nodes, {time.time() - t0:.1f}s){abl}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions(a) + objects(a)
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps({"format": 2, "levels": sols}))
    C.emit(GID, gdy, {
        "name": "Rack Packer",
        "tagline": "Sokoban with GPUs: cool them, belt them, rack them.",
        "description": "Bufo installs GPU cards by pushing them into empty rack slots. Hot cards must slide "
                       "through a coolant puddle first or the rack goes up in flames, and Mistral-logo conveyor "
                       "belts whisk cards across the datacenter one tile per tick.",
        "tags": ["meme", "puzzle", "sokoban"],
        "difficulty": "medium",
        "original": "",
        "accent": "#f97316",
        "icon": "📦",
        "controls": {"move": "move", "extra": [{"key": "z", "label": "Nap", "action": "nap", "directional": False}]},
        "hud": [{"label": "Empty slots", "kind": "global", "variable": "open", "tone": "amber"},
                {"label": "Hot cards", "kind": "count", "object": "hotgpu", "tone": "rose"},
                {"label": "Cool cards", "kind": "count", "object": "gpu", "tone": "sky"}],
        "win_text": "Every rack is full. Ship it!",
        "lose_text": "You racked a hot GPU. Fire in aisle three!",
    })
    if "--diff" in sys.argv:
        diff()


def snapshot(s: Sim):
    me, cool, hot, slots = s.st
    lv = s.lv
    objs = {"bufo": [list(me)]}
    if cool:
        objs["gpu"] = sorted(list(c) for c in cool)
    if hot:
        objs["hotgpu"] = sorted(list(c) for c in hot)
    if slots:
        objs["slot"] = sorted(list(c) for c in slots)
    return {"objs": objs, "open": len(slots), "term": s.result}


def diff(n_random=12, length=150):
    rng = random.Random(3)
    sols = json.loads(SOLUTIONS.read_text())["levels"]
    jobs = []
    for i in range(len(LEVELS)):
        jobs.append({"level": i, "actions": sols[i]["actions"]})
        for _ in range(n_random):
            jobs.append({"level": i, "actions": [["move", rng.randint(1, 4)] if rng.random() < 0.85 else ["nap", 1]
                                                 for _ in range(length)]})
    for j in jobs:
        j["keep"] = ["bufo", "gpu", "hotgpu", "slot"]
    with tempfile.TemporaryDirectory() as td:
        jp, op = Path(td) / "jobs.json", Path(td) / "out.json"
        jp.write_text(json.dumps(jobs))
        subprocess.run(["node", str(HERE / "meme_dump.mjs"), str(C.GAMES_DIR / f"{GID}.yaml"), str(jp), str(op)],
                       check=True, stdout=subprocess.DEVNULL)
        out = json.loads(op.read_text())
    bad = 0
    for job, states in zip(jobs, out):
        s = Sim(Level(LEVELS[job["level"]][3]))
        for t, eng in enumerate(states):
            if t > 0:
                s.step(*job["actions"][t - 1])
            mine = snapshot(s)
            e = {"objs": eng["objs"], "open": eng["globals"]["open"], "term": eng["term"]}
            if mine != e:
                bad += 1
                print(f"  DIFF level {job['level']} step {t} {job['actions'][t - 1] if t else None}: mine {mine} "
                      f"engine {e}", file=sys.stderr)
                break
    total = sum(len(s) for s in out)
    print(f"diff: {len(jobs)} runs, {total} states compared, {bad} mismatching runs", file=sys.stderr)
    if bad:
        sys.exit(1)


if __name__ == "__main__":
    main()
