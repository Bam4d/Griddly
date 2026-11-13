#!/usr/bin/env python3
"""Bufo vs the Benchmaxxers: a top-down action arena in a datacenter. A new "meme" game (no original).

Leaderboard trophies keep spawning 'benchmaxx' bots that overfit straight at bufo. Bufo throws mayo:
`throw` (directional, Space) spawns a mayo blob in the cell in front, and it flies on one cell per tick
until it hits something. A splatted bot leaves a mayo puddle; bots that step into a puddle get stuck
for a while. Each trophy takes 3 mayo hits; destroy every trophy to win. GPU pickups give rapid fire
for 40 ticks, mayo jars heal one HP. Every destroyed trophy enrages the rest (they spawn faster and
the hunters think faster).

Rules (all deterministic for a given seed: no Triggers, no Probability; randomness only via exec
Randomize, which uses the seeded engine RNG):
  * bufo (avatar) has 3 HP. A bot that touches bufo (either way) costs 1 HP and splats itself.
  * throw: needs `ready` = 1; it sets ready = 0 and a `reload` re-arms after 5 ticks (1 tick with rapid
    fire). Throwing straight at an adjacent bot / trophy hits it at once.
  * mayo (projectile): InitialActions `fly` with no ActionId, so the first flight reuses the throw's
    direction (spawn-vector fallback), then `fly` re-execs itself without ActionId (keeps the vector).
  * wanderers (b, blue) roam at random; hunters (h, red) chase bufo with A* (`exec` + `Search`).
  * trophies: bronze T spawns wanderers (~every 34 ticks), gold Y spawns hunters (~every 40), SOTA Z
    spawns hunters fast (~every 22). Spawning stops while `alive` (global bot count) >= the cap.

Layout characters (besides the shared terrain chars from common, unused here):
  W wall  R server rack  M Mistral logo plinth  T/Y/Z bronze/gold/SOTA trophy  h hunter bot
  b wanderer bot  g GPU (rapid fire)  j mayo jar (+1 HP)  A bufo  c cable (floor decal)  . floor
Every level is validated (rectangular, one bufo, every trophy/pickup reachable, every trophy has a free
neighbour to spawn into); bot_benchmaxx.mjs proves each level winnable and records the winning runs.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

GID = "benchmaxx"
P = M.PALETTE
WALK = set(".c")
SOLID = "WRM"
TROPHIES = {"T": "trophy", "Y": "trophy_gold", "Z": "trophy_sota"}


# ----------------------------------------------------------------------------- sprites (original art)
def _trophy(metal, dmg, label_col):
    hi, mid, dark = metal
    px = M.blank()
    # base + stem
    M.rect(px, 6, 19, 17, 21, dark)
    M.rect(px, 7, 18, 16, 18, mid)
    M.rect(px, 7, 19, 16, 19, hi)
    M.rect(px, 10, 14, 13, 17, mid)
    M.rect(px, 10, 14, 10, 17, hi)
    # handles
    for hx in (3, 18):
        M.ellipse(px, hx + 1.5, 7.5, 2.6, 3.2, dark)
        M.ellipse(px, hx + 1.5, 7.5, 1.3, 1.9, M.T)
    # bowl
    M.ellipse(px, 12, 6, 7.5, 8.5, mid, hi, dark)
    for y in range(0, 3):
        for x in range(24):
            px[y][x] = M.T
    M.rect(px, 4, 3, 19, 3, hi)
    # "#1" label
    for x, y in [(9, 6), (9, 7), (9, 8), (9, 9), (8, 7), (10, 7), (8, 8), (10, 8), (12, 6), (13, 6), (13, 7), (13, 8),
                 (13, 9), (12, 9), (14, 9)]:
        M.put(px, x, y, label_col)
    M.outline(px)
    if dmg >= 1:  # cracks
        M.line(px, 15, 4, 12, 9, P["black"])
        M.line(px, 12, 9, 14, 12, P["black"])
        for x, y in ((16, 2), (17, 1)):
            M.put(px, x, y, P["mayo"])
    if dmg >= 2:
        M.line(px, 7, 5, 10, 11, P["black"])
        M.line(px, 10, 11, 8, 14, P["black"])
        M.rect(px, 5, 4, 7, 5, P["mayo"])
        M.rect(px, 14, 12, 16, 13, P["mayo"])
        M.put(px, 3, 13, P["mayo_dark"])
    M.ground_shadow(px, ry=1.0, cy=22.6, rx=7)
    return px


def trophy_sprites(kind):
    metal = {"trophy": ((240, 180, 120, 255), (205, 127, 50, 255), (140, 80, 30, 255)),
             "trophy_gold": (P["mistral_yellow"], P["gold"], P["gold_dark"]),
             "trophy_sota": ((235, 245, 255, 255), (170, 200, 225, 255), (100, 120, 150, 255))}[kind]
    label = {"trophy": (90, 40, 10, 255), "trophy_gold": P["mistral_red"], "trophy_sota": P["led_blue"]}[kind]
    out = []
    for d in range(3):
        px = _trophy(metal, d, label)
        if kind == "trophy_sota":  # sparkle
            for x, y in ((2, 2), (21, 1), (20, 4)):
                M.put(px, x, y, P["white"])
        out.append(M.save(f"benchmaxx-{kind}-{d}", px))
    return out


def _bot(body, eye, frame=0, stuck=False):
    px = M.blank()
    oy = frame
    # antenna
    M.line(px, 12, 1 + oy, 12, 4 + oy, P["steel_dark"])
    M.put(px, 12, 0 + oy, eye)
    # head (screen with an up-only chart)
    M.rect(px, 4, 4 + oy, 19, 14 + oy, body[1])
    M.rect(px, 4, 4 + oy, 19, 4 + oy, body[0])
    M.rect(px, 4, 4 + oy, 4, 14 + oy, body[0])
    M.rect(px, 19, 5 + oy, 19, 14 + oy, body[2])
    M.rect(px, 6, 6 + oy, 17, 12 + oy, (16, 20, 30, 255))
    for i, h in enumerate((1, 2, 3, 5, 6)):
        M.rect(px, 7 + i * 2, 12 + oy - h, 7 + i * 2, 12 + oy - 1, P["led_green"] if i < 4 else eye)
    # eyes
    M.rect(px, 7, 7 + oy, 8, 7 + oy, eye)
    M.rect(px, 15, 7 + oy, 16, 7 + oy, eye)
    # body + legs
    M.rect(px, 7, 15 + oy, 16, 19, body[1])
    M.rect(px, 7, 15 + oy, 16, 15 + oy, body[0])
    M.rect(px, 9, 16 + oy, 14, 17 + oy, P["steel_dark"])
    M.put(px, 10, 16 + oy, eye)
    for lx in (8, 14):
        M.rect(px, lx, 20, lx + 1, 21, P["steel_dark"])
    M.outline(px)
    if stuck:
        for x, y in ((6, 20), (7, 21), (16, 21), (17, 20), (11, 21), (12, 21), (13, 21), (5, 21), (18, 21)):
            M.put(px, x, y, P["mayo"])
    M.ground_shadow(px, ry=1.0, cy=22.6, rx=7)
    return px


def bot_sprites(kind):
    if kind == "hunter":
        body, eye = ((236, 100, 100, 255), (200, 50, 60, 255), (120, 24, 36, 255)), P["led_red"]
    else:
        body, eye = ((130, 200, 250, 255), (60, 140, 220, 255), (30, 80, 140, 255)), P["led_blue"]
    return [M.save(f"benchmaxx-{kind}-{i}", _bot(body, eye, frame=i % 2, stuck=i == 2)) for i in range(3)]


def mayo_blob():
    px = M.blank()
    M.ellipse(px, 12, 12, 5.0, 4.4, P["mayo"], P["mayo_hi"], P["mayo_dark"])
    M.ellipse(px, 8, 13, 2.0, 1.8, P["mayo"])
    M.ellipse(px, 16.5, 10.5, 1.6, 1.4, P["mayo"])
    M.put(px, 11, 10, P["white"])
    M.put(px, 12, 10, P["white"])
    M.outline(px, P["mayo_dark"])
    return M.save("benchmaxx-mayo-shot", px)


def plinth_logo():
    px = M.blank()
    M.rect(px, 1, 18, 22, 23, P["steel_dark"])
    M.rect(px, 1, 18, 22, 18, P["steel_hi"])
    logo = C.load_png(C.IMAGES_DIR / M.mistral_logo())[2]
    small = [[logo[min(23, int(y * 24 / 18))][min(23, int(x * 24 / 18))] for x in range(18)] for y in range(18)]
    for y in range(18):
        for x in range(18):
            if small[y][x][3]:
                M.put(px, x + 3, y, small[y][x])
    return M.save("benchmaxx-plinth", px)


# ----------------------------------------------------------------------------- levels
class Arena(C.Map):
    def aisle(self, x0, y0, x1, y1, gaps=()):
        """A row/column of server racks with gaps at the given coordinates."""
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if (x if y0 == y1 else y) not in gaps:
                    self.set(x, y, "R")

    def cables(self, n):
        for _ in range(n):
            x, y = self.rng.randint(2, self.w - 3), self.rng.randint(2, self.h - 3)
            horiz = self.rng.random() < 0.5
            for k in range(self.rng.randint(3, 7)):
                p = (x + k, y) if horiz else (x, y + k)
                if self.inside(*p) and self.get(*p) == ".":
                    self.set(*p, "c")


def lobby():
    m = Arena(26, 16, seed=21, border="W")
    m.aisle(8, 4, 8, 11, gaps=(7, 8))
    m.aisle(17, 4, 17, 11, gaps=(7, 8))
    m.set(12, 7, "M")
    m.set(13, 7, "M")
    m.set(3, 8, "A")
    m.set(22, 3, "T")
    m.set(22, 12, "T")
    m.set(12, 12, "b")
    m.set(20, 8, "b")
    m.set(12, 3, "g")
    m.set(4, 13, "j")
    m.cables(5)
    return m


def eval_harness():
    m = Arena(30, 18, seed=22, border="W")
    for x in (7, 14, 21):
        m.aisle(x, 2, x, 6, gaps=(4,))
        m.aisle(x, 11, x, 15, gaps=(13,))
    m.set(14, 8, "M")
    m.set(14, 9, "M")
    m.set(2, 9, "A")
    m.set(26, 3, "T")
    m.set(26, 14, "T")
    m.set(18, 9, "Y")
    m.set(10, 3, "b")
    m.set(24, 9, "b")
    m.set(10, 14, "g")
    m.set(3, 2, "j")
    m.set(17, 14, "j")
    m.cables(7)
    return m


def contamination_lab():
    m = Arena(32, 20, seed=23, border="W")
    for y in (5, 14):
        m.aisle(3, y, 28, y, gaps=(6, 7, 15, 16, 24, 25))
    m.rect(13, 8, 18, 11, "R", filled=False)
    for p in [(13, 9), (13, 10), (18, 9), (18, 10)]:
        m.set(*p, ".")
    m.set(15, 9, "M")
    m.set(16, 10, "M")
    m.set(15, 17, "A")
    m.set(4, 2, "T")
    m.set(27, 2, "Y")
    m.set(4, 10, "Y")
    m.set(27, 10, "T")
    m.set(16, 2, "h")
    m.set(9, 9, "b")
    m.set(22, 9, "b")
    m.set(16, 9, "g")
    m.set(2, 17, "j")
    m.set(29, 17, "j")
    m.set(10, 2, "g")
    m.cables(8)
    return m


def arena_hard():
    m = Arena(34, 21, seed=24, border="W")
    for x in (6, 27):
        m.aisle(x, 3, x, 17, gaps=(9, 10, 11))
    for y in (6, 14):
        m.aisle(10, y, 23, y, gaps=(16, 17))
    m.set(16, 10, "M")
    m.set(17, 10, "M")
    m.set(2, 10, "A")
    for p, t in [((11, 3), "T"), ((22, 3), "Y"), ((31, 3), "Y"), ((31, 17), "T"), ((16, 17), "Y")]:
        m.set(*p, t)
    m.set(12, 10, "b")
    m.set(22, 10, "h")
    m.set(30, 10, "b")
    m.set(3, 3, "g")
    m.set(3, 17, "g")
    m.set(20, 10, "j")
    m.set(13, 17, "j")
    m.set(24, 3, "j")
    m.cables(9)
    return m


def sota_or_bust():
    m = Arena(36, 22, seed=25, border="W")
    m.aisle(8, 2, 8, 19, gaps=(5, 6, 15, 16))
    m.aisle(27, 2, 27, 19, gaps=(5, 6, 15, 16))
    m.aisle(12, 7, 23, 7, gaps=(17, 18))
    m.aisle(12, 14, 23, 14, gaps=(17, 18))
    m.set(17, 10, "M")
    m.set(18, 11, "M")
    m.set(3, 11, "A")
    for p, t in [((13, 3), "Y"), ((22, 3), "Y"), ((32, 3), "Z"), ((32, 18), "Z"), ((13, 18), "T"), ((22, 18), "Y")]:
        m.set(*p, t)
    m.set(17, 4, "h")
    m.set(30, 11, "h")
    m.set(17, 17, "h")
    m.set(22, 11, "b")
    m.set(13, 11, "b")
    m.set(4, 3, "g")
    m.set(4, 18, "g")
    m.set(17, 11, "g")
    m.set(2, 2, "j")
    m.set(2, 19, "j")
    m.set(30, 7, "j")
    m.cables(10)
    return m


LEVELS = [
    ("Leaderboard Lobby", "Two bronze trophies and a few wandering bots. Learn to throw mayo (Space).", lobby),
    ("Eval Harness", "A gold trophy spawns red hunters that path straight to you. Puddles slow them down.",
     eval_harness),
    ("Contamination Lab", "Four trophies behind two rack aisles. Grab the GPUs for rapid fire.", contamination_lab),
    ("Arena Hard", "Five trophies, three of them gold. Every trophy you break enrages the rest.", arena_hard),
    ("SOTA or Bust", "☠ Two SOTA trophies flood the floor with hunters. Very aggressive. Good luck, bufo.",
     sota_or_bust),
]


def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one bufo"
    seen = C.reach(rows, a[0], lambda c, p: c in WALK or c in "Abhgj")
    adj = lambda p: [(p[0] + dx, p[1] + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
    for ch in "TYZ":
        for p in C.find(rows, ch):
            assert any(q in seen for q in adj(p)), f"{name}: trophy {ch} at {p} unreachable"
            assert sum(rows[q[1]][q[0]] in WALK for q in adj(p)) >= 2, f"{name}: trophy {ch} at {p} boxed in"
    for ch in "gjhb":
        bad = [p for p in C.find(rows, ch) if p not in seen]
        assert not bad, f"{name}: {ch} sealed off at {bad}"
    ax, ay = a[0]
    near = [p for c in "hbTYZ" for p in C.find(rows, c) if abs(p[0] - ax) + abs(p[1] - ay) <= 5]
    assert not near, f"{name}: enemies too close to the start {near}"
    return {k: len(C.find(rows, c)) for k, c in
            (("bronze", "T"), ("gold", "Y"), ("sota", "Z"), ("hunters", "h"), ("wanderers", "b"), ("gpus", "g"),
             ("jars", "j"))}


# ----------------------------------------------------------------------------- GDY
BOTS = ["hunter", "wanderer"]
TROPHY_OBJS = list(TROPHIES.values())
SCENERY = C.SCENERY_NAMES + ["wall", "rack", "plinth"]
FLOOR = "[_empty, " + ", ".join(C.GROUND_NAMES + ["cable"]) + "]"
ALL = C.GROUND_NAMES + ["cable"] + SCENERY + TROPHY_OBJS + BOTS + ["puddle", "gpu", "jar", "mayo", "bufo"]
CAP = {"trophy": 8, "trophy_gold": 9, "trophy_sota": 12}
RATE = {"trophy": 34, "trophy_gold": 40, "trophy_sota": 22}
SPAWN = {"trophy": "wanderer", "trophy_gold": "hunter", "trophy_sota": "hunter"}


def lst(names):
    return "[" + ", ".join(names) + "]"


def splat_dst():
    """Dst commands for a bot that gets splatted (mayo hit / kamikaze into bufo)."""
    return "[{decr: alive}, {change_to: puddle}]"


def trophy_hit():
    return ("[{decr: hp}, {incr: dmg}, {set_tile: dmg}, "
            "{if: {Conditions: {lte: [hp, 0]}, OnTrue: [{incr: rage}, {decr: trophies}, {remove: true}]}}]")


def hurt_bufo():
    return "[{decr: hp}, {if: {Conditions: {lte: [hp, 0]}, OnTrue: [{remove: true}]}}]"


def actions():
    me = "bufo"
    face = "{set_tile: meta.facing}"
    reload_ = ("{if: {Conditions: {gt: [rapid, 0]}, OnTrue: [{exec: {Action: reload, ActionId: 1, Delay: 1}}], "
               "OnFalse: [{exec: {Action: reload, ActionId: 1, Delay: 5}}]}}")
    fire = f"{face}, {{set: [ready, 0]}}, {reload_}"
    blocked_for_bots = [o for o in ALL if o not in C.GROUND_NAMES + ["cable", "puddle", "mayo", me]]
    a_star_block = [o for o in ALL if o not in C.GROUND_NAMES + ["cable", "puddle", "mayo", me]]
    roam_ex = lambda d: f"{{exec: {{Action: roam, Delay: {d}, Randomize: true}}}}"
    hatch = ""
    for t in TROPHY_OBJS:
        rate = RATE[t]
        hatch += f'''      - Src:
          Object: {t}
          Preconditions: [{{lt: [alive, {CAP[t]}]}}]
          Commands: [{{spawn: {SPAWN[t]}}}, {{set: [rate, {rate}]}}, {{sub: [rate, rage]}}, {{sub: [rate, rage]}}, {{sub: [rate, rage]}}, {{exec: {{Action: hatch, Delay: rate, Randomize: true}}}}]
        Dst: {{Object: {FLOOR}}}
      - Src:
          Object: {t}
          Commands: [{{exec: {{Action: hatch, Delay: 9, Randomize: true}}}}]
        Dst: {{Object: {lst([o for o in ALL if o not in C.GROUND_NAMES + ["cable"]])}}}
      - Src:
          Object: {t}
          Preconditions: [{{gte: [alive, {CAP[t]}]}}]
          Commands: [{{exec: {{Action: hatch, Delay: 9, Randomize: true}}}}]
        Dst: {{Object: {FLOOR}}}
'''
    return f'''
Actions:
{C.face_action(me)}
  # Every bot counts itself into the global `alive` when it appears (spawn caps read it).
  - Name: count_bot
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {lst(BOTS)}, Commands: [{{incr: alive}}]}}
        Dst: {{Object: {lst(BOTS + ["mayo"])}}}
      - Src: {{Object: {lst(TROPHY_OBJS)}, Commands: [{{incr: trophies}}]}}
        Dst: {{Object: {lst(TROPHY_OBJS + ["mayo"])}}}

  # Trophies hatch a bot into a random neighbouring floor cell, faster with every trophy destroyed
  # (global `rage`), and only while fewer than the cap of bots are alive.
  - Name: hatch
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
{hatch}
  # Wanderers roam at random. Stepping into a mayo puddle sticks them for 12 ticks.
  - Name: roam
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: wanderer, Commands: [{{mov: _dest}}, {{set_tile: 0}}, {roam_ex(4)}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: wanderer, Commands: [{{mov: _dest}}, {{set_tile: 2}}, {roam_ex(14)}]}}
        Dst: {{Object: puddle}}
      - Src: {{Object: wanderer, Commands: [{roam_ex(4)}]}}
        Dst: {{Object: {lst(blocked_for_bots)}}}
      - Src: {{Object: wanderer, Commands: [{{decr: alive}}, {{change_to: puddle}}]}}
        Dst: {{Object: mayo, Commands: [{{remove: true}}]}}
      - Src: {{Object: wanderer, Commands: [{{decr: alive}}, {{change_to: puddle}}]}}
        Dst: {{Object: {me}, Commands: {hurt_bufo()}}}

  # Publishes bufo's position (globals ax / ay) for the hunters' range check.
  - Name: sync
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Sync, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {me}, Commands: [{{set: [ax, _x]}}, {{set: [ay, _y]}}]}}
        Dst: {{Object: {lst([me, "mayo"])}}}

  # Hunter think, every `pace` ticks (3, 2 once two trophies are down) plus `slow` after a puddle:
  # within 14 tiles of bufo take one A* step towards it (`hunt` with Search), else wander.
  - Name: stalk
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: hunter
          Commands:
            - set: [rx, _x]
            - sub: [rx, ax]
            - if: {{Conditions: {{lt: [rx, 0]}}, OnTrue: [{{set: [d, 0]}}, {{sub: [d, rx]}}], OnFalse: [{{set: [d, rx]}}]}}
            - set: [ry, _y]
            - sub: [ry, ay]
            - if: {{Conditions: {{lt: [ry, 0]}}, OnTrue: [{{sub: [d, ry]}}], OnFalse: [{{add: [d, ry]}}]}}
            - set: [pace, 3]
            - if: {{Conditions: {{gte: [rage, 2]}}, OnTrue: [{{set: [pace, 2]}}]}}
            - add: [pace, slow]
            - set: [slow, 0]
            - exec: {{Action: stalk, ActionId: 1, Delay: pace}}
            - if:
                Conditions: {{lte: [d, 14]}}
                OnTrue: [{{exec: {{Action: hunt, Search: {{TargetObjectName: {me}, ImpassableObjects: {lst(a_star_block)}}}}}}}]
                OnFalse: [{{exec: {{Action: hunt, Randomize: true}}}}]
        Dst: {{Object: {lst(["hunter", "mayo"])}}}

  # One hunter step (absolute 1 W, 2 N, 3 E, 4 S), chosen by A* or at random.
  - Name: hunt
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: hunter, Commands: [{{mov: _dest}}, {{set_tile: 0}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: hunter, Commands: [{{mov: _dest}}, {{set_tile: 2}}, {{set: [slow, 9]}}]}}
        Dst: {{Object: puddle}}
      - Src: {{Object: hunter, Commands: [{{decr: alive}}, {{change_to: puddle}}]}}
        Dst: {{Object: mayo, Commands: [{{remove: true}}]}}
      - Src: {{Object: hunter, Commands: [{{decr: alive}}, {{change_to: puddle}}]}}
        Dst: {{Object: {me}, Commands: {hurt_bufo()}}}

  # The mayo blob flies one cell per tick in the throw direction until it hits something.
  - Name: fly
    InputMapping:
      Internal: true
    Behaviours:
      - Src: {{Object: mayo, Commands: [{{mov: _dest}}, {{exec: {{Action: fly, Delay: 1}}}}]}}
        Dst: {{Object: {FLOOR[:-1]}, puddle, gpu, jar, {me}]}}
      - Src: {{Object: mayo, Commands: [{{remove: true}}]}}
        Dst: {{Object: {lst(BOTS)}, Commands: {splat_dst()}}}
      - Src: {{Object: mayo, Commands: [{{remove: true}}]}}
        Dst: {{Object: {lst(TROPHY_OBJS)}, Commands: {trophy_hit()}}}
      - Src: {{Object: mayo, Commands: [{{remove: true}}]}}
        Dst: {{Object: {lst(SCENERY + ["mayo"])}}}

  # Re-arm after a throw; rapid fire (GPU) wears off.
  - Name: reload
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Reload, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {me}, Commands: [{{set: [ready, 1]}}]}}
        Dst: {{Object: {lst([me, "mayo"])}}}
  - Name: rapid_off
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Cool down, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {me}, Commands: [{{decr: rapid}}]}}
        Dst: {{Object: {lst([me, "mayo"])}}}

  # Bufo walks up / down / left / right; GPUs give rapid fire, mayo jars heal.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{exec: {{Action: sync, ActionId: 1}}}}]}}
        Dst: {{Object: {FLOOR[:-1]}, puddle, mayo]}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{exec: {{Action: sync, ActionId: 1}}}}, {{incr: rapid}}, {{reward: 1}}, {{exec: {{Action: rapid_off, ActionId: 1, Delay: 40}}}}]}}
        Dst: {{Object: gpu, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{lt: [src.hp, 3]}}]
          Commands: [{face}, {{mov: _dest}}, {{exec: {{Action: sync, ActionId: 1}}}}, {{incr: hp}}, {{reward: 1}}]
        Dst: {{Object: jar, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.hp, 3]}}]
          Commands: [{face}]
        Dst: {{Object: jar}}
      - Src: {{Object: {me}, Commands: [{face}, {{decr: hp}}, {{if: {{Conditions: {{lte: [hp, 0]}}, OnTrue: [{{remove: true}}]}}}}]}}
        Dst: {{Object: {lst(BOTS)}, Commands: {splat_dst()}}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst(SCENERY + TROPHY_OBJS)}}}

  # Throw mayo in a direction (the UI sends the last move direction). Needs `ready`.
  - Name: throw
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.ready, 1]}}]
          Commands: [{fire}, {{spawn: mayo}}]
        Dst: {{Object: {FLOOR[:-1]}, puddle, gpu, jar]}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.ready, 1]}}]
          Commands: [{fire}, {{reward: 1}}]
        Dst: {{Object: {lst(BOTS)}, Commands: {splat_dst()}}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.ready, 1]}}]
          Commands: [{fire}, {{reward: 2}}]
        Dst: {{Object: {lst(TROPHY_OBJS)}, Commands: {trophy_hit()}}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst(SCENERY + ["mayo"])}}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.ready, 0]}}]
          Commands: [{face}]
        Dst: {{Object: {lst([o for o in ALL if o not in SCENERY + ["mayo", me]])}}}
'''


def objects():
    avatar = M.bufo("plain")
    v = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: {iv}\n" for n, iv in names)
    bufo_extra = v([("hp", 3), ("ready", 1), ("rapid", 0)]) + C.FACE_INITIAL_ACTION + "      - Action: sync\n        ActionId: 1\n"
    count = "      - Action: count_bot\n        ActionId: 1\n"
    hunter_extra = (v([("rx", 0), ("ry", 0), ("d", 0), ("pace", 3), ("slow", 0)]) +
                    "    InitialActions:\n" + count + "      - Action: stalk\n        ActionId: 1\n        Delay: 4\n")
    wander_extra = "    InitialActions:\n" + count + "      - Action: roam\n        Delay: 4\n        Randomize: true\n"
    trophy_extra = (v([("hp", 3), ("dmg", 0), ("rate", 30)]) +
                    "    InitialActions:\n" + count + "      - Action: hatch\n        Delay: 20\n        Randomize: true\n")
    mayo_extra = "    InitialActions:\n      - Action: fly\n        Delay: 1\n"
    rack_anim, rack_init = M.animate("rack", 2, 3, start_tile=0)
    out = [
        C.obj("cable", "c", 0, C.img(M.cable()), "[0.3, 0.5, 0.9]", "square", 0.5),
        C.obj("wall", "W", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("rack", "R", 1, C.imgs(M.rack_frames()), "[0.2, 0.2, 0.25]", "square", 0.95, rack_init, tiles=2),
        C.obj("plinth", "M", 1, C.img(plinth_logo()), "[1.0, 0.55, 0.0]", "square", 0.9),
        C.obj("puddle", None, 1, C.img(M.mayo_splat()), "[0.95, 0.92, 0.75]", "circle", 0.7),
        C.obj("gpu", "g", 1, C.img(M.gpu("cool")), "[0.2, 0.9, 0.3]", "square", 0.6),
        C.obj("jar", "j", 1, C.img(M.mayo_jar()), "[1.0, 1.0, 0.8]", "square", 0.6),
    ]
    blk = {"trophy": "[0.8, 0.5, 0.2]", "trophy_gold": "[1.0, 0.8, 0.1]", "trophy_sota": "[0.7, 0.85, 1.0]"}
    for ch, t in TROPHIES.items():
        out.append(C.obj(t, ch, 1, C.imgs(trophy_sprites(t)), blk[t], "triangle", 0.9, trophy_extra, tiles=3))
    out += [
        C.obj("hunter", "h", 2, C.imgs(bot_sprites("hunter")), "[0.9, 0.2, 0.2]", "triangle", 0.7, hunter_extra, tiles=3),
        C.obj("wanderer", "b", 2, C.imgs(bot_sprites("wanderer")), "[0.2, 0.5, 0.9]", "triangle", 0.7, wander_extra,
              tiles=3),
        C.obj("bufo", "A", 2, C.imgs(avatar), "[0.4, 0.7, 0.25]", "triangle", 0.9, bufo_extra, tiles=4),
        C.obj("mayo", None, 3, C.img(mayo_blob()), "[1.0, 1.0, 0.85]", "circle", 0.4, mayo_extra),
    ]
    return "\nObjects:\n" + C.terrain_objects() + "".join(out), rack_anim


def main():
    variables = ("    - Name: trophies\n      InitialValue: 0\n    - Name: alive\n      InitialValue: 0\n    - Name: rage\n      InitialValue: 0\n"
                 "    - Name: ax\n      InitialValue: 0\n    - Name: ay\n      InitialValue: 0\n")
    win = "lte: [trophies, 0]"
    parts = [C.header("Bufo vs the Benchmaxxers", """
        Leaderboard trophies spawn benchmaxx bots that rush bufo. Throw mayo (directional) - it flies until
        it hits something. Splatted bots leave sticky mayo puddles that trap other bots for a while. Each
        trophy takes 3 hits; destroy them all to win. Red hunters path-find to bufo (A*), blue bots
        wander. GPUs give rapid fire, mayo jars heal. Bufo has 3 HP; every bot that touches it costs one.
        """.replace("        ", ""), "bufo", [win], ["eq: [bufo:count, 0]"],
        background=M.datacenter_floor(), variables=variables)]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m = build()
        rows = m.rows(solid="W")
        info = validate(name, rows)
        parts.append(C.level_block(name, blurb, rows))
        print(f"{i}. {name:18s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    objs, rack_anim = objects()
    gdy = "".join(parts) + actions() + rack_anim + objs
    C.emit(GID, gdy, {
        "name": "Bufo vs the Benchmaxxers",
        "tagline": "Splat leaderboard-chasing bots with mayo and smash their trophies.",
        "description": "Benchmaxx bots pour out of leaderboard trophies and swarm bufo. Throw mayo to splat them "
                       "(the puddles trap their friends), grab GPUs for rapid fire and smash every trophy - three "
                       "hits each - before they overfit you to death.",
        "tags": ["meme", "action", "arena", "shooter"],
        "difficulty": "hard",
        "original": "",
        "accent": "#f59e0b",
        "icon": "🏆",
        "controls": {"move": "move", "extra": [{"key": " ", "label": "Throw mayo", "action": "throw",
                                                "directional": True}]},
        "hud": [{"label": "HP", "kind": "var", "object": "bufo", "variable": "hp", "tone": "rose"},
                {"label": "Bronze", "kind": "count", "object": "trophy", "tone": "orange"},
                {"label": "Gold", "kind": "count", "object": "trophy_gold", "tone": "amber"},
                {"label": "SOTA", "kind": "count", "object": "trophy_sota", "tone": "sky"},
                {"label": "Bots", "kind": "global", "variable": "alive", "tone": "violet"},
                {"label": "Rapid fire", "kind": "var", "object": "bufo", "variable": "rapid", "tone": "lime"}],
        "win_text": "Every leaderboard trophy is mayo'd. Bufo is the true SOTA!",
        "lose_text": "Overfitted. The benchmaxxers got bufo.",
    })


if __name__ == "__main__":
    main()
