#!/usr/bin/env python3
"""Mayo Rush: a sliding/pushing puzzle about a toad who can't stop eating mayo (new "meme" game).

Rules (the GDY below; `Sim` mirrors them exactly and the BFS solver proves every level solvable)
  * Bufo moves with absolute 4-way `move`. Walking into a mayo jar (`j`) eats it: bufo steps into its
    cell, gets one tier bigger (tier 0 -> 3, shown by the sprite) and leaves a mayo splat behind.
  * Tier >= 2 ("chonky"): mayo splats (`s`, pre-spilled or left by eaten jars) are as slippery as ice -
    bufo slides on in the same direction until something stops it (it stops ON the first non-splat
    floor cell, or before a wall / rack / door it can't fit through; sliding into a jar eats it and stops).
  * Tier 3 ("absolute unit"): bufo can push heavy server racks (`R`) one cell (onto floor or splats).
    Pushing never slides. Bolted racks (`S`) never move.
  * Doors are too narrow for a fat toad: a narrow door (`n`) only lets bufo through at tier <= 1, a
    regular door (`N`) at tier <= 2. Bufo never gets thinner, so eat in the right order.
  * Win: reach (eat) the giant golden mayo jar (`J`). There is no lose condition - restart (R) if stuck.

Layout characters
  w wall  A bufo  j mayo jar  J golden mayo jar  s mayo splat  n narrow door  N regular door
  R heavy rack (pushable at tier 3)  S bolted rack  k coolant tank  P potted plant  M Mistral logo sign
  c cable (floor decal)  m Mistral logo floor decal  v vent  . raised floor
  ~ mayo spill wildcard (splat or a stopper), filled from a fixed seed by `fill()`.

Validation: every level is solved by a BFS over (bufo cell, eaten jars, rack cells) with the exact
rules (`Sim`); the shortest solution is written to apps/web/src/games/solutions/mayorush.json (plain
move lists, replay_solutions.mjs checks them in the engine). Ablations print whether each level is
still solvable (and how long it takes) when doors ignore tiers / splats aren't slippery / racks are
pushable at any tier, as evidence that the mechanics matter.
"""
from __future__ import annotations

import collections
import json
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

GID = "mayorush"
HERE = Path(__file__).resolve().parent
SOLUTIONS = HERE.parents[1] / "apps/web/src/games/solutions" / f"{GID}.json"
P = M.PALETTE
DV = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
FLOOR = set(".cmv")          # plain walkable floor (decals included)
SOLID = set("wSkPM")
DOOR_MAX = {"n": 1, "N": 2}


# ----------------------------------------------------------------------------- art (original, memekit)
def _scale(px, sx, sy, ax=12.0, ay=23.0):
    out = M.blank()
    for y in range(24):
        for x in range(24):
            u = int((x + 0.5 - ax) / sx + ax)
            v = int((y + 0.5 - ay) / sy + ay)
            if 0 <= u < 24 and 0 <= v < 24:
                out[y][x] = px[v][u]
    return out


def bufo_tiers():
    """16 render tiles: tier t facing f -> tile 4t + f (f: up, right, down, left)."""
    looks = {"up": (0, -1), "right": (1, 0), "down": (0, 1), "left": (-1, 0)}
    spec = [("plain", 0.66, 0.66), ("plain", 0.84, 0.84), ("mayo", 1.04, 0.98), ("mayo", 1.3, 1.08)]
    out = []
    for t, (variant, sx, sy) in enumerate(spec):
        for d in M.FACINGS:
            body = _scale(M._bufo_body(variant, looks[d]), sx, sy)
            if t == 3:  # mayo dribbles + a crown of shine on the absolute unit
                for x, y in ((3, 20), (20, 19), (6, 22), (17, 22)):
                    M.put(body, x, y, P["mayo"])
            out.append(M.save(f"mayorush-bufo-t{t}-{d}", M.add_arrow(body, d)))
    return out


def golden_jar():
    frames = []
    for shine, name in ((0, "a"), (1, "b")):
        px = M._jar(shine)
        for y in range(24):
            for x in range(24):
                p = px[y][x]
                if p[3] and p != P["outline"] and p[:3] != (24, 22, 28):
                    lum = (p[0] + p[1] + p[2]) / 765
                    px[y][x] = (min(255, int(120 + 150 * lum)), min(255, int(80 + 140 * lum)), int(20 + 60 * lum), 255)
        for sx, sy in ((2, 3), (21, 6), (20, 1)) if shine else ((3, 8), (21, 2), (1, 1)):
            M.put(px, sx, sy, P["white"])
            M.put(px, sx + 1, sy, P["mistral_yellow"])
            M.put(px, sx, sy + 1, P["mistral_yellow"])
        frames.append(M.save(f"mayorush-golden-{name}", px))
    return frames


def door(max_tier):
    px = M.blank()
    gap = {1: 3, 2: 6}[max_tier]          # half-width of the opening
    M.rect(px, 0, 0, 23, 23, (40, 34, 30, 255))
    M.rect(px, 0, 0, 23, 2, P["mistral_orange"])
    M.rect(px, 0, 0, 23, 0, P["mistral_yellow"])
    post = (110, 116, 130, 255)
    M.rect(px, 0, 3, 12 - gap - 1, 23, post)
    M.rect(px, 12 + gap, 3, 23, 23, post)
    M.rect(px, 12 - gap - 1, 3, 12 - gap - 1, 23, P["steel_dark"])
    M.rect(px, 12 + gap, 3, 12 + gap, 23, P["steel_hi"])
    for i in range(max_tier):                 # size pips: how many tiers fit
        M.rect(px, 9 + i * 4, 6, 10 + i * 4, 7, P["led_green"])
    for y in range(9, 23, 3):                 # hazard stripes on the posts
        M.rect(px, 1, y, 12 - gap - 3, y, P["mistral_amber"])
        M.rect(px, 12 + gap + 2, y, 22, y, P["mistral_amber"])
    return M.save(f"mayorush-door{max_tier}", px)


def plant():
    px = M.blank()
    for cx, cy, r in ((12, 8, 5), (7, 10, 4), (17, 10, 4), (10, 5, 3), (15, 5, 3)):
        M.ellipse(px, cx, cy, r, r * 0.9, (60, 150, 70, 255), (110, 200, 100, 255), (34, 96, 46, 255))
    M.rect(px, 7, 14, 16, 21, (200, 110, 60, 255))
    M.rect(px, 6, 13, 17, 14, (230, 140, 80, 255))
    M.rect(px, 15, 15, 16, 21, (160, 80, 40, 255))
    M.outline(px)
    return M.save("mayorush-plant", px)


def vent():
    px = M.blank(P["floor_dark"])
    M.rect(px, 0, 0, 23, 0, P["floor"])
    M.rect(px, 0, 0, 0, 23, P["floor"])
    for y in range(3, 22, 3):
        M.rect(px, 3, y, 20, y, (60, 64, 76, 255))
        M.rect(px, 3, y + 1, 20, y + 1, (96, 102, 116, 255))
    return M.save("mayorush-vent", px)


# ----------------------------------------------------------------------------- simulation + BFS
class Sim:
    """Exact replica of the GDY rules. State = (pos, eaten bitmask, racks frozenset)."""

    def __init__(self, rows, doors_ignore_tier=False, no_slide=False, push_any=False):
        self.rows = rows
        self.H, self.W = len(rows), len(rows[0])
        self.jars = {p: i for i, p in enumerate(C.find(rows, "j"))}
        self.start = C.find(rows, "A")[0]
        self.golden = set(C.find(rows, "J"))
        self.racks0 = frozenset(C.find(rows, "R"))
        self.splat0 = set(C.find(rows, "s"))
        self.abl = (doors_ignore_tier, no_slide, push_any)

    def ch(self, p):
        x, y = p
        if not (0 <= x < self.W and 0 <= y < self.H):
            return "w"
        return self.rows[y][x]

    def tier(self, eaten):
        return min(3, bin(eaten).count("1"))

    def step(self, state, a):
        """-> (new_state, won) or None when nothing happens."""
        pos, eaten, racks = state
        dx, dy = DV[a]
        tier = self.tier(eaten)
        q = (pos[0] + dx, pos[1] + dy)
        c = self.ch(q)
        doors_ign, no_slide, push_any = self.abl
        if q in self.golden:
            return (q, eaten, racks), True
        if q in racks:
            if tier < 3 and not push_any:
                return None
            r2 = (q[0] + dx, q[1] + dy)
            c2 = self.ch(r2)
            if r2 in racks or r2 in self.golden or c2 in SOLID or c2 in DOOR_MAX or c2 == "J":
                return None
            if c2 == "j" and not (eaten >> self.jars[r2]) & 1:
                return None
            return (q, eaten, (racks - {q}) | {r2}), False
        if c in SOLID:
            return None
        if c == "j" and not (eaten >> self.jars[q]) & 1:
            return (q, eaten | (1 << self.jars[q]), racks), False
        if c in DOOR_MAX:
            if tier > DOOR_MAX[c] and not doors_ign:
                return None
            return (q, eaten, racks), False
        # floor or splat
        if self.is_splat(q, eaten) and tier >= 2 and not no_slide:
            return self.slide(q, eaten, racks, dx, dy)
        return (q, eaten, racks), False

    def is_splat(self, p, eaten):
        return p in self.splat0 or (p in self.jars and (eaten >> self.jars[p]) & 1)

    def slide(self, pos, eaten, racks, dx, dy):
        tier = self.tier(eaten)
        while True:
            q = (pos[0] + dx, pos[1] + dy)
            c = self.ch(q)
            if q in self.golden:
                return (q, eaten, racks), True
            if q in racks or c in SOLID:
                return (pos, eaten, racks), False
            if c == "j" and not (eaten >> self.jars[q]) & 1:
                return (q, eaten | (1 << self.jars[q]), racks), False
            if c in DOOR_MAX:
                if tier > DOOR_MAX[c] and not self.abl[0]:
                    return (pos, eaten, racks), False
                return (q, eaten, racks), False
            if self.is_splat(q, eaten):
                pos = q
                continue
            return (q, eaten, racks), False

    def solve(self, max_states=4_000_000):
        s0 = (self.start, 0, self.racks0)
        prev = {s0: None}
        q = collections.deque([s0])
        while q:
            s = q.popleft()
            for a in (1, 2, 3, 4):
                r = self.step(s, a)
                if r is None:
                    continue
                ns, won = r
                if won:
                    path = [a]
                    while prev[s] is not None:
                        s, a2 = prev[s]
                        path.append(a2)
                    return path[::-1], len(prev)
                if ns in prev:
                    continue
                prev[ns] = (s, a)
                q.append(ns)
            if len(prev) > max_states:
                break
        return None, len(prev)

    def traps(self, max_states=2_000_000):
        """(reachable states, dead states = reachable but the golden jar is no longer reachable)."""
        s0 = (self.start, 0, self.racks0)
        succ = {}
        q = collections.deque([s0])
        succ[s0] = []
        winners = set()
        while q and len(succ) < max_states:
            s = q.popleft()
            for a in (1, 2, 3, 4):
                r = self.step(s, a)
                if r is None:
                    continue
                ns, won = r
                if won:
                    winners.add(s)
                    continue
                succ[s].append(ns)
                if ns not in succ:
                    succ[ns] = []
                    q.append(ns)
        pred = collections.defaultdict(list)
        for s, ns in succ.items():
            for n in ns:
                pred[n].append(s)
        good = set(winners)
        q = collections.deque(winners)
        while q:
            s = q.popleft()
            for p in pred[s]:
                if p not in good:
                    good.add(p)
                    q.append(p)
        return len(succ), len(succ) - len(good)

    def stats(self, path):
        s = (self.start, 0, self.racks0)
        slides = pushes = 0
        for a in path:
            ns, won = self.step(s, a)
            if ns[2] != s[2]:
                pushes += 1
            d = abs(ns[0][0] - s[0][0]) + abs(ns[0][1] - s[0][1])
            if d > 1:
                slides += 1
            s = ns
        return dict(eaten=bin(s[1]).count("1"), slides=slides, pushes=pushes)


# ----------------------------------------------------------------------------- levels
# Levels are hand-drawn; the spill wildcards of some are filled from a fixed seed (picked offline by
# scanning seeds for the most slides / dead ends with the BFS below):
#   ~  mayo spill: splat, sometimes a stopper (plant / coolant / bolted rack)
#   ?  floor, sometimes a stopper or a heavy rack
#   %  heavy rack or floor
SPILL = [("s", 0.86), ("P", 0.05), ("k", 0.04), ("S", 0.05)]
MAYBE = [(".", 0.80), ("S", 0.08), ("R", 0.08), ("P", 0.04)]
RACKISH = [("R", 0.6), (".", 0.4)]


def fill(rows, seed):
    rng = random.Random(seed)
    table = {"~": SPILL, "?": MAYBE, "%": RACKISH}
    out = []
    for r in rows:
        line = []
        for c in r:
            if c in table:
                x, acc = rng.random(), 0.0
                for ch, pr in table[c]:
                    acc += pr
                    if x < acc:
                        break
                line.append(ch)
            else:
                line.append(c)
        out.append("".join(line))
    return out


# (name, blurb, art, fill seed or None)
LEVEL_ART = [
    ("First Bite", "Every jar makes bufo bigger. Slim toads fit the narrow door; only an absolute unit (tier 3) can shove a rack.",
     """
wwwwwwwwwwwwwwwwwwwwwwww
wk.......w............Pw
w........w....j........w
w..A.....w.......wwwwwww
w........w.......w....Jw
w...j....n.......R.....w
w........w.......wwwwwww
w..j.....w..........m..w
wP.......w.....j.......w
w.c......w.............w
w...m....w...j....M....w
w........w.............w
wk.......w...........ckw
wwwwwwwwwwwwwwwwwwwwwwww
""", None),
    ("Mayo Rink", "Chonky toads (tier 2+) slide on mayo like ice. Bulk up, skate across and shove the rack.",
     """
wwwwwwwwwwwwwwwwwwwwwwwwwwww
wk.....w~~~~~~~~~~~~~~~w...w
w......w~~~~~~~~~~~~~~~w.c.w
w..j...w~~~~~~~~~~~~~~~w...w
w.A....~~~~~~~~~~~~~~~~R...w
w......w~~~~~~~~~~~~~~~wJ..w
w......w~~~~~~~~~~~~~~~www.w
wc..j..~~~~~~~~~~~~~~~~w...w
w......w~~~~~~~~~~~~~~~wwwww
w..m...w~~~~~~~~~~~~~~~w...w
w......w~~~~~~~~~~~~~~~w.m.w
w....j.~~~~~~~~~~~~~~~~w...w
wP.....w~~~~~~~~~~~~~~~w...w
wk.....w~~~~~~~~~~~~~~~wM.kw
wwwwwwwwwwwwwwwwwwwwwwwwwwww
""", 82),
    ("Rack and Roll", "A server room packed with heavy racks. Get big enough to push, then push in the right order.",
     """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
wk.......w...........w........kw
w........w..j..m.....w.SSSSSSS.w
w..A.....w...........w.........w
w........w..SSSSSSS..w.SSS.SSS.w
w...j....n...........N.........w
w........w..SSSRSSS..wwwwRwwwwww
wP.......w...R....j..w...R.....w
w..c.....w....R......R...R..S..w
w.....m..wSSSSRSSS..wwR....SSJ.w
wk.......w..j........wwwwSwwwwww
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
""", None),
    ("Diet Plan", "Narrow doors (tier 1 max), regular doors (tier 2 max), then the mayo rink. Plan every snack.",
     """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
wk......w.......w~~~~~~~~~~~~~~..w
w.......w...j...w~~~~~~~~~~~~~~.jw
w..A....n.......N~~~~~~~~~~~~~~..w
w.......w.......w~~~~~~~~~~~~~~..w
w...j...w..m....w~~~~~~~~~~~~~~..w
wwwwnwwwwwwwNwwww~~~~~~~~~~~~~~..w
w.......w.......w~~~~~~~~~~~~~~..w
w...j...w...j...w~~~~~~~~~~~~~~..w
w.......n.......N~~~~~~~~~~~~~~..w
w..c....w.......w~~~~~~~~~~~~~~..w
wP......w....M..w~~~~~~~~~~~~~~..w
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwRwwww
wwwwwwwwwwwwwwwwwwwwwwwwwk.......w
wwwwwwwwwwwwwwwwwwwwwwwwwP....J.Mw
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
""", 39),
    ("The Golden Mayo", "Doors, slides and racks all at once. The giant golden mayo jar awaits the absolute unit.",
     """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
wk......w.........wsssssssssssssskkw
w.......w...j.....wssssPsssssssssssw
w.......w.........wsssssssPssssssssw
w..A....n.........NssssssssssssSsssw
w.......w.....m...wsPsssssssssssssjw
w.......w.........wsssssPssssssssssw
w.......w..m......wssssssssssssksssw
w...j...w.........wsssssssssssPssssw
w.......w....M....wwwwwwwwwwwwwwwwRw
w..j....wwwwwwwwwwwww......S.....w.w
w.......wwwwwwwwwwwwJ..R..R....R...w
wP...c..wwwwwwwwwwwwk...R..S...R...w
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
""", None),
]


def build_levels():
    out = []
    for name, blurb, art, seed in LEVEL_ART:
        rows = [r for r in art.strip("\n").splitlines()]
        if seed is not None:
            rows = fill(rows, seed)
        C.check_rect(name, rows)
        out.append((name, blurb, rows))
    return out


# ----------------------------------------------------------------------------- GDY
FLOOR_OBJS = ["cable", "logo_floor", "vent"]
WALK = "[_empty, " + ", ".join(FLOOR_OBJS) + "]"
SOLIDS = "[wall, bolted, coolant, plant, logo_sign]"


def face():
    """Remember the facing (slides have no MetaData) and show tile facing + 4 * tier."""
    return "{set: [facing, meta.facing]}, {set: [tile, facing]}, {add: [tile, tb]}, {set_tile: tile}"


def grow():
    """Eat a jar: one tier bigger (max 3), retile."""
    return ("{incr: eaten}, {if: {Conditions: {lt: [tier, 3]}, OnTrue: [{incr: tier}, {add: [tb, 4]}]}}, "
            "{set: [tile, facing]}, {add: [tile, tb]}, {set_tile: tile}, {reward: 1}")


def actions():
    gold_anim, _ = M.animate("golden", 2, 4)
    jar_anim, _ = M.animate("jar", 2, 5)
    rack_anim, _ = M.animate("rack", 2, 3)
    logo_anim, _ = M.animate("logo_sign", 4, 3)
    common_slide = f'''      - Src: {{Object: bufo, Commands: [{{set: [won, 1]}}, {{reward: 10}}]}}
        Dst: {{Object: golden}}
      - Src: {{Object: bufo, Preconditions: [{{lte: [src.tier, 1]}}], Commands: [{{mov: _dest}}]}}
        Dst: {{Object: door_narrow}}
      - Src: {{Object: bufo, Preconditions: [{{lte: [src.tier, 2]}}], Commands: [{{mov: _dest}}]}}
        Dst: {{Object: door_wide}}'''
    return f'''
Actions:
{C.face_action("bufo")}
  # Bufo: absolute 4-way move. Render tile = facing + 4 * tier (tb = 4 * tier).
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: bufo, Commands: [{face()}, {{mov: _dest}}]}}
        Dst: {{Object: {WALK}}}
      - Src: {{Object: bufo, Preconditions: [{{lt: [src.tier, 2]}}], Commands: [{face()}, {{mov: _dest}}]}}
        Dst: {{Object: splat}}
      - Src: {{Object: bufo, Preconditions: [{{gte: [src.tier, 2]}}], Commands: [{face()}, {{mov: _dest}}, {{exec: {{Action: slide}}}}]}}
        Dst: {{Object: splat}}
      # eating: the jar turns into a splat, bufo steps onto it and grows (no slide)
      - Src: {{Object: bufo, Commands: [{face()}, {{mov: _dest}}, {grow()}]}}
        Dst: {{Object: jar, Commands: [{{change_to: splat}}]}}
      - Src: {{Object: bufo, Commands: [{face()}, {{set: [won, 1]}}, {{reward: 10}}]}}
        Dst: {{Object: golden}}
      - Src: {{Object: bufo, Preconditions: [{{lte: [src.tier, 1]}}], Commands: [{face()}, {{mov: _dest}}]}}
        Dst: {{Object: door_narrow}}
      - Src: {{Object: bufo, Preconditions: [{{gt: [src.tier, 1]}}], Commands: [{face()}]}}
        Dst: {{Object: door_narrow}}
      - Src: {{Object: bufo, Preconditions: [{{lte: [src.tier, 2]}}], Commands: [{face()}, {{mov: _dest}}]}}
        Dst: {{Object: door_wide}}
      - Src: {{Object: bufo, Preconditions: [{{gt: [src.tier, 2]}}], Commands: [{face()}]}}
        Dst: {{Object: door_wide}}
      # pushing a heavy rack needs tier 3 (the rack moves one cell; pushing never slides)
      - Src: {{Object: bufo, Preconditions: [{{gte: [src.tier, 3]}}], Commands: [{face()}, {{mov: _dest}}]}}
        Dst: {{Object: rack, Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: bufo, Preconditions: [{{lt: [src.tier, 3]}}], Commands: [{face()}]}}
        Dst: {{Object: rack}}
      - Src: {{Object: bufo, Commands: [{face()}]}}
        Dst: {{Object: {SOLIDS}}}
      - Src: {{Object: rack, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: [_empty, cable, logo_floor, vent, splat]}}

  # Internal: keep sliding in the same direction (exec without ActionId reuses the move's vector).
  - Name: slide
    InputMapping:
      Internal: true
    Behaviours:
      - Src: {{Object: bufo, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {WALK}}}
      - Src: {{Object: bufo, Commands: [{{mov: _dest}}, {{exec: {{Action: slide}}}}]}}
        Dst: {{Object: splat}}
      - Src: {{Object: bufo, Commands: [{{mov: _dest}}, {grow()}]}}
        Dst: {{Object: jar, Commands: [{{change_to: splat}}]}}
{common_slide}

{gold_anim}{jar_anim}{rack_anim}{logo_anim}'''


def objects():
    v = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: {iv}\n" for n, iv in names)
    bufo_extra = v([("tier", 0), ("tb", 0), ("tile", 2), ("eaten", 0), ("facing", 2)]) + C.FACE_INITIAL_ACTION
    _, gold_init = M.animate("golden", 2, 4)
    _, rack_init = M.animate("rack", 2, 3)
    _, jar_init = M.animate("jar", 2, 5)
    out = [
        C.obj("wall", "w", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("cable", "c", 0, C.img(M.cable()), "[0.2, 0.5, 0.95]", "circle", 0.5),
        C.obj("logo_floor", "m", 0, C.img(M.mistral_logo()), "[1.0, 0.5, 0.0]", "square", 0.6),
        C.obj("vent", "v", 0, C.img(vent()), "[0.4, 0.42, 0.5]", "square", 1.0),
        C.obj("splat", "s", 0, C.img(M.mayo_splat()), "[0.98, 0.95, 0.8]", "circle", 0.9),
        C.obj("door_narrow", "n", 0, C.img(door(1)), "[0.9, 0.4, 0.1]", "square", 0.5),
        C.obj("door_wide", "N", 0, C.img(door(2)), "[0.9, 0.6, 0.1]", "square", 0.8),
        C.obj("bolted", "S", 1, C.img(M.server_rack("idle")), "[0.25, 0.25, 0.3]", "square", 1.0),
        C.obj("coolant", "k", 1, C.img(M.coolant()), "[0.3, 0.8, 0.9]", "square", 0.8),
        C.obj("plant", "P", 1, C.img(plant()), "[0.2, 0.7, 0.3]", "circle", 0.8),
        C.obj("logo_sign", "M", 1, C.imgs(M.mistral_logo_spin()), "[1.0, 0.6, 0.0]", "square", 0.9,
              M.animate("logo_sign", 4, 3)[1], tiles=4),
        C.obj("rack", "R", 1, C.imgs(M.rack_frames()), "[0.5, 0.5, 0.6]", "square", 1.0, rack_init, tiles=2),
        C.obj("jar", "j", 1, C.imgs(M.mayo_jar_frames()), "[1.0, 1.0, 0.85]", "square", 0.7, jar_init, tiles=2),
        C.obj("golden", "J", 1, C.imgs(golden_jar()), "[1.0, 0.8, 0.1]", "square", 1.0, gold_init, tiles=2),
        C.obj("bufo", "A", 1, C.imgs(bufo_tiers()), "[0.4, 0.7, 0.25]", "triangle", 0.9, bufo_extra, tiles=16),
    ]
    return "\nObjects:\n" + "".join(out)


def diff(levels, walks=40, length=120):
    """Random walks: the engine (mayorush_dump.mjs) must match Sim after every move."""
    import subprocess
    import tempfile
    rng = random.Random(7)
    jobs = []
    for li, (_, _, rows) in enumerate(levels):
        for _ in range(walks):
            jobs.append({"level": li, "moves": [rng.choice((1, 2, 3, 4, 4, 3)) for _ in range(length)]})
    with tempfile.TemporaryDirectory() as d:
        jf, of = Path(d, "jobs.json"), Path(d, "out.json")
        jf.write_text(json.dumps(jobs))
        subprocess.run(["node", str(HERE / "mayorush_dump.mjs"), str(C.GAMES_DIR / f"{GID}.yaml"), str(jf), str(of)],
                       check=True, capture_output=True)
        res = json.loads(of.read_text())
    bad = 0
    for job, states in zip(jobs, res):
        sim = Sim(levels[job["level"]][2])
        s = (sim.start, 0, sim.racks0)
        for k, (a, st) in enumerate(zip(job["moves"], states[1:])):
            r = sim.step(s, a)
            won = False
            if r is not None:
                s, won = r
            exp = (list(s[0]), sim.tier(s[1]), sorted(map(list, s[2])), won)
            got = (st["pos"], st["tier"], sorted(st["racks"]), st["won"])
            if exp != got and not won:
                print(f"  DIFF level {job['level']} move {k}: sim {exp} engine {got}", file=sys.stderr)
                bad += 1
                break
            if won:
                break
    print(f"diff: {len(jobs)} random walks, {bad} mismatches", file=sys.stderr)
    return bad


def main():
    levels = build_levels()
    variables = "    - Name: won\n      InitialValue: 0\n"
    hdr = C.header("Mayo Rush", """
        Bufo eats mayo. Every jar makes bufo one tier bigger (0..3) and leaves a mayo splat. At tier 2+
        splats are slippery as ice; at tier 3 bufo can push heavy server racks; but fat toads don't fit
        through narrow doors (narrow: tier <= 1, regular: tier <= 2). Eat in the right order and reach the
        giant golden mayo jar.
        """.replace("        ", ""), "bufo", ["eq: [won, 1]"], [], background=M.datacenter_floor(),
        variables=variables)
    parts = [hdr]
    sols = []
    for i, (name, blurb, rows) in enumerate(levels):
        C.check_rect(name, rows)
        assert len(C.find(rows, "A")) == 1 and len(C.find(rows, "J")) >= 1, name
        sim = Sim(rows)
        path, n = sim.solve()
        assert path, f"{name}: unsolvable ({n} states)"
        st = sim.stats(path)
        abl = []
        for label, kw in (("doors ignore tier", dict(doors_ignore_tier=True)), ("no slide", dict(no_slide=True)),
                          ("push at any tier", dict(push_any=True))):
            p2, _ = Sim(rows, **kw).solve()
            abl.append(f"{label}: {len(p2) if p2 else 'UNSOLVABLE'}")
        nst, dead = sim.traps()
        print(f"{i}. {name:18s} {len(rows[0])}x{len(rows)} jars={len(sim.jars)} racks={len(sim.racks0)} "
              f"BFS: {len(path)} moves {st} | {nst}{'+ (capped)' if nst >= 2_000_000 else ''} states, "
              f"{dead} dead ends "
              f"| ablations: {', '.join(abl)}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
        sols.append(path)
        parts.append(C.level_block(name, blurb, rows))
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Mayo Rush",
        "tagline": "Every jar makes bufo bigger. Eat in the right order.",
        "description": "A puzzle about a toad who can't stop eating mayo. Each jar makes bufo a tier fatter: chonky "
                       "toads slide on mayo splats like ice, absolute units can shove server racks, but nobody that "
                       "big fits through the narrow doors. Plan your snacks and reach the golden mayo jar.",
        "tags": ["meme", "puzzle", "sliding", "push"],
        "difficulty": "medium",
        "original": "",
        "accent": "#fde047",
        "icon": "🥫",
        "controls": {"move": "move", "extra": []},
        "hud": [
            {"label": "Tier", "kind": "var", "object": "bufo", "variable": "tier", "tone": "amber"},
            {"label": "Jars eaten", "kind": "var", "object": "bufo", "variable": "eaten", "tone": "lime"},
            {"label": "Jars left", "kind": "count", "object": "jar", "tone": "sky"},
        ],
        "win_text": "The golden mayo jar is bufo's. Absolute unit achieved.",
        "lose_text": "Stuck! Restart and rethink the snack order.",
    })
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps(sols))
    print(f"wrote {SOLUTIONS.relative_to(C.ROOT)}", file=sys.stderr)
    if "--diff" in sys.argv:
        assert diff(levels) == 0


if __name__ == "__main__":
    main()
