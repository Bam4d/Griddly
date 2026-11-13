#!/usr/bin/env python3
"""Deep Delve: an isometric mining game. Dig through dirt and rock, mine ore, dodge cave-ins and lava.

A new game (no original) drawn mainly with ISOMETRIC cameras (Isometric whole map, IsoFollow tracking
the miner); it also has the usual top-down Sprite2D / FollowCam / CloseUp cameras.

Rules
  * `dig` (directional, Space) hits the block in front: dirt breaks at once, rock needs two hits (it
    cracks first), an ore vein breaks and drops 1 ore into your pack. Every hit costs 1 stamina; a
    ration restores some. A dug cell becomes a mine tunnel.
  * Each level is split into chambers by bedrock walls with one gate each: a short dirt tube whose
    middle cell is plain rock, sits under a hanging boulder, or touches a lava pocket. Dig the plug and
    brace it with a beam at once (the boulder wobbles for a few ticks first).
  * The mine slopes downhill to the south. A boulder with an empty cell south of it wobbles for a few
    ticks and then rolls south until something stops it. A rolling boulder crushes the miner; a
    resting one does not. Walking into a boulder from the side shoves it one cell (east/west only).
  * Lava pockets ooze into dug tunnels (never into natural cave floor) and burn the miner.
  * `build` (directional, B) puts a timber support beam into the free cell in front (costs 1 timber;
    timber piles give 2). Beams stop rolling boulders and lava but the miner can walk through them.
  * Walk into loose ore to pick it up. Walk into an empty ore cart with 3 ore to fill it. When every
    cart is full the lift works: walk into it to win.

Layout characters (besides the shared terrain chars from common):
  A miner  W bedrock  d dirt  r rock  v ore vein  O boulder  L lava  n old tunnel  o loose ore
  t timber  f ration  c ore cart  E lift  . cave floor
Every level is validated (rectangular, one miner, lift and carts reachable by digging, enough ore for
the carts, no boulder starts unsupported); bot_minecrawler.mjs proves each level winnable in the
engine and records the winning runs (solutions/minecrawler.json).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import isokit as I  # noqa: E402

GID = "minecrawler"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
TORCH, STONE, ROCKS = S["torch"], S["stone"], S["rock"]
DIGGABLE = set("drv")
WALK = set(".nftoA") | set(G.values())
CART_ORE = 3
STAMINA = 30
RATION = 15


# ----------------------------------------------------------------------------- level painting
class Mine(C.Map):
    def __init__(self, w, h, seed):
        super().__init__(w, h, seed, fill="d", border="W")

    def cave(self, cx, cy, r, jitter=0.3):
        self.blob(cx, cy, r, ".", jitter=jitter)

    def tunnel(self, p0, p1, ch="."):
        """Straight L-shaped corridor (4-connected), x first."""
        (x0, y0), (x1, y1) = p0, p1
        for x in range(min(x0, x1), max(x0, x1) + 1):
            self.set(x, y0, ch)
        for y in range(min(y0, y1), max(y0, y1) + 1):
            self.set(x1, y, ch)

    def sprinkle(self, ch, n, only="d", region=None, cluster=1):
        x0, y0, x1, y1 = region or (1, 1, self.w - 2, self.h - 2)
        cells = [(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if self.get(x, y) in only]
        self.rng.shuffle(cells)
        placed = 0
        for x, y in cells:
            if placed >= n:
                break
            self.set(x, y, ch)
            placed += 1
            for _ in range(cluster - 1):
                dx, dy = self.rng.choice(((1, 0), (-1, 0), (0, 1), (0, -1)))
                if self.inside(x + dx, y + dy) and self.get(x + dx, y + dy) in only:
                    self.set(x + dx, y + dy, ch)

    def boulders(self, n, region=None):
        """Boulders on dirt, each resting on something solid (dirt/rock/vein/bedrock/boulder)."""
        x0, y0, x1, y1 = region or (1, 1, self.w - 2, self.h - 3)
        cells = [(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1)
                 if self.get(x, y) == "d" and self.get(x, y + 1) in "drvW"]
        self.rng.shuffle(cells)
        for p in cells[:n]:
            self.set(*p, "O")

    def lava(self, cx, cy, r):
        """A lava pocket with a rock rim (dig the rim and the lava follows you). Never within 2 cells of
        a tunnel (gates stay single hazards)."""
        near = {(x + dx, y + dy) for y in range(self.h) for x in range(self.w) if self.get(x, y) == "n"
                for dx in range(-2, 3) for dy in range(-2, 3)}
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                d = ((x - cx) ** 2 + ((y - cy) * 1.15) ** 2) ** 0.5
                if self.inside(x, y) and (x, y) not in near and d <= r and self.get(x, y) in "drv":
                    self.set(x, y, "L")
        for y in range(1, self.h - 1):
            for x in range(1, self.w - 1):
                if self.get(x, y) == "L":
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        if self.get(x + dx, y + dy) == "d":
                            self.set(x + dx, y + dy, "r")


def chambers(m, walls, gates):
    """Split the mine into chambers with 3-wide vertical bedrock walls (at x..x+2) and cut one gate per
    wall: a dirt tube at row y whose middle cell (the plug) is under a boulder hanging in a notch
    ("boulder"), next to a lava pocket ("lava") or plain rock ("rock")."""
    for x in walls:
        m.rect(x, 1, x + 2, m.h - 2, "W")
    for x, y, kind in gates:
        m.set(x, y, "d")
        m.set(x + 1, y, "r" if kind == "rock" else "d")
        m.set(x + 2, y, "d")
        if kind == "boulder":
            m.set(x + 1, y - 1, "O")
        elif kind == "lava":
            m.set(x + 1, y + 1, "L")
            m.set(x + 1, y + 2, "L")
        # the cells just outside the tube are open so the gate is easy to find
        m.set(x - 1, y, "n")
        m.set(x + 3, y, "n")


def furnish(m, region, rock=8, veins=3, boulders=4, lava=()):
    x0, y0, x1, y1 = region
    for cx, cy, r in lava:
        m.lava(cx, cy, r)
    m.sprinkle("r", rock, cluster=3, region=region)
    m.sprinkle("v", veins, cluster=2, region=region)
    m.boulders(boulders, region=(x0, y0, x1, y1 - 1))


def shallow_seam():
    m = Mine(22, 14, seed=21)
    chambers(m, [10], [(10, 7, "rock")])
    m.cave(4, 4, 2.4)
    m.cave(16, 9, 2.4)
    furnish(m, (1, 1, 9, 12), rock=5, veins=2, boulders=3)
    furnish(m, (13, 1, 20, 12), rock=5, veins=3, boulders=3)
    a = (3, 4)
    m.set(*a, "A")
    m.set(18, 10, "E")
    m.set(16, 11, "c")
    for p in [(5, 3), (4, 6)]:
        m.set(*p, "o")
    m.set(2, 5, "t")
    m.set(15, 7, TORCH)
    m.set(5, 2, TORCH)
    return m, a, dict(dirt=3, bones=2, skull=1, mush=1)


def boulder_gallery():
    m = Mine(26, 16, seed=22)
    chambers(m, [8, 17], [(8, 6, "boulder"), (17, 10, "rock")])
    m.cave(4, 3, 2.0)
    m.cave(13, 8, 2.0)
    m.cave(22, 12, 2.0)
    furnish(m, (1, 1, 7, 14), rock=5, veins=2, boulders=5)
    furnish(m, (11, 1, 16, 14), rock=5, veins=3, boulders=5)
    furnish(m, (20, 1, 24, 14), rock=4, veins=3, boulders=4)
    a = (3, 3)
    m.set(*a, "A")
    m.set(23, 13, "E")
    m.set(21, 14, "c")
    m.set(23, 11, "c")
    for p in [(13, 9), (4, 4)]:
        m.set(*p, "o")
    m.set(2, 2, "t")
    m.set(12, 9, "f")
    for p in [(14, 7), (21, 11), (5, 2)]:
        m.set(*p, TORCH)
    return m, a, dict(dirt=4, bones=3, skull=2, sand=1)


def magma_pockets():
    m = Mine(26, 17, seed=23)
    chambers(m, [8, 17], [(8, 11, "lava"), (17, 5, "boulder")])
    m.cave(3.5, 13, 2.0)
    m.cave(12.5, 4, 2.0)
    m.cave(22, 13, 2.0)
    furnish(m, (1, 1, 7, 15), rock=5, veins=3, boulders=3, lava=[(4, 6, 1.0)])
    furnish(m, (11, 1, 16, 15), rock=5, veins=3, boulders=4, lava=[(13, 11, 1.1)])
    furnish(m, (20, 1, 24, 15), rock=4, veins=3, boulders=3, lava=[(22, 6, 1.0)])
    a = (3, 13)
    m.set(*a, "A")
    m.set(23, 14, "E")
    m.set(21, 12, "c")
    m.set(22, 15, "c")
    m.set(13, 3, "o")
    m.set(2, 12, "t")
    m.set(5, 14, "t")
    m.set(12, 5, "f")
    for p in [(11, 3), (20, 14), (5, 12)]:
        m.set(*p, TORCH)
    return m, a, dict(dirt=3, bones=3, skull=3, sand=2)


def cave_in_crossing():
    m = Mine(30, 19, seed=24)
    chambers(m, [9, 19], [(9, 4, "boulder"), (19, 14, "lava")])
    m.cave(4, 4, 2.2)
    m.cave(14, 9, 2.4)
    m.cave(25, 15, 2.2)
    furnish(m, (1, 1, 8, 17), rock=7, veins=3, boulders=7, lava=[(4, 12, 1.2)])
    furnish(m, (12, 1, 18, 17), rock=7, veins=4, boulders=7, lava=[(15, 15, 1.0)])
    furnish(m, (22, 1, 28, 17), rock=7, veins=4, boulders=7, lava=[(25, 5, 1.2)])
    a = (3, 4)
    m.set(*a, "A")
    m.set(27, 16, "E")
    for p in [(24, 16), (26, 13), (27, 14)]:
        m.set(*p, "c")
    for p in [(14, 9), (5, 5)]:
        m.set(*p, "o")
    for p in [(2, 5), (4, 6), (15, 10)]:
        m.set(*p, "t")
    for p in [(13, 10), (24, 14)]:
        m.set(*p, "f")
    for p in [(12, 8), (16, 8), (5, 2), (28, 14)]:
        m.set(*p, TORCH)
    return m, a, dict(dirt=5, bones=4, skull=3, sand=2, mush=2)


def the_deep_lift():
    m = Mine(32, 20, seed=25)
    chambers(m, [8, 16, 24], [(8, 15, "boulder"), (16, 5, "lava"), (24, 12, "boulder")])
    m.cave(4, 16, 2.0)
    m.cave(12, 5, 2.0)
    m.cave(20, 11, 2.0)
    m.cave(28, 15, 1.8)
    furnish(m, (1, 1, 7, 18), rock=7, veins=3, boulders=6, lava=[(4, 6, 1.1)])
    furnish(m, (11, 1, 15, 18), rock=6, veins=3, boulders=6, lava=[(13, 13, 1.1)])
    furnish(m, (19, 1, 23, 18), rock=6, veins=3, boulders=6, lava=[(21, 16, 1.0)])
    furnish(m, (27, 1, 30, 18), rock=5, veins=3, boulders=5)
    a = (3, 16)
    m.set(*a, "A")
    m.set(28, 17, "E")
    for p in [(27, 17), (29, 15), (29, 17)]:
        m.set(*p, "c")
    for p in [(12, 4), (20, 12)]:
        m.set(*p, "o")
    for p in [(5, 17), (13, 6)]:
        m.set(*p, "t")
    for p in [(11, 5), (14, 16), (21, 10), (22, 3), (29, 13)]:
        m.set(*p, "f")
    for p in [(2, 15), (11, 3), (19, 10), (28, 13)]:
        m.set(*p, TORCH)
    return m, a, dict(dirt=6, bones=5, skull=4, sand=2, mush=2)


LEVELS = [
    ("Shallow Seam", "Dig through the rock seam, fill one ore cart, ride the lift.", shallow_seam),
    ("Boulder Gallery", "A boulder hangs over the gate: dig, then brace it with a beam!", boulder_gallery),
    ("Magma Pockets", "Lava oozes into any tunnel dug next to it. Beams hold it back.", magma_pockets),
    ("Cave-In Crossing", "Three carts, a hanging boulder and a lava gate.", cave_in_crossing),
    ("The Deep Lift", "Three gates, four chambers: ration your stamina and timber.", the_deep_lift),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one miner"
    assert len(C.find(rows, "E")) == 1, f"{name}: need one lift"
    carts = C.find(rows, "c")
    assert carts, f"{name}: need at least one cart"
    seen = C.reach(rows, a[0], lambda c, p: c in WALK or c in DIGGABLE)
    adj = lambda p: any((p[0] + dx, p[1] + dy) in seen for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    for ch in "cEvo":
        bad = [p for p in C.find(rows, ch) if not adj(p) and p not in seen]
        assert not bad, f"{name}: unreachable {ch} at {bad}"
    ore = len(C.find(rows, "o")) + len(C.find(rows, "v"))
    assert ore >= CART_ORE * len(carts) + 2, f"{name}: only {ore} ore for {len(carts)} carts"
    for x, y in C.find(rows, "O"):
        assert rows[y + 1][x] not in ".n" + "".join(G.values()), f"{name}: boulder at {(x, y)} starts unsupported"
    # every hazardous gate (a plug under a boulder or next to lava) needs a beam: count timber
    need = sum(1 for x, y in C.find(rows, "d") if rows[y - 1][x] == "O" and rows[y][x - 1] in "dn" and rows[y][x + 1] in "dn"
               and rows[y + 1][x] == "W" and rows[y - 1][x - 1] == "W")
    need += sum(1 for x, y in C.find(rows, "d") if rows[y + 1][x] == "L" and rows[y - 1][x] == "W")
    assert 2 * len(C.find(rows, "t")) >= need, f"{name}: {need} hazardous gates but not enough timber"
    return dict(gates_needing_beams=need, carts=len(carts), ore=ore, boulders=len(C.find(rows, "O")), lava=len(C.find(rows, "L")),
                rock=len(C.find(rows, "r")), dirt=len(C.find(rows, "d")))


# ----------------------------------------------------------------------------- GDY
SCENERY = ", ".join(C.SCENERY_NAMES)
FLOOR = C.EMPTY_OR_GROUND[:-1] + ", tunnel]"
SOLIDS = ["bedrock", "dirt_block", "rock_block", "vein", "boulder", "lava", "ore", "timber", "ration", "cart",
          "cart_full", "lift"]
ALL_TOP = C.SCENERY_NAMES + SOLIDS + ["beam", "miner"]


def lst(names):
    return "[" + ", ".join(names) + "]"


def actions():
    me = "miner"
    face = "{set_tile: meta.facing}"
    fall = lambda d: f"{{exec: {{Action: fall, ActionId: 1, Delay: {d}}}}}"
    flow = "{exec: {Action: flow, Delay: 7, Randomize: true}}"
    blocked_fall = lst([o for o in ALL_TOP if o != me])
    blocked_flow = lst([o for o in ALL_TOP if o != me] + [g for g in C.GROUND_NAMES] + ["_empty"])
    stam = "{gte: [src.stamina, 1]}"
    tunnel = "{spawn: tunnel}"
    return f'''
Actions:
{C.face_action(me)}
  # Ore carts count themselves into the level goal at reset.
  - Name: count_goal
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: cart, Commands: [{{incr: carts_needed}}]}}
        Dst: {{Object: cart}}

  # Gravity: every boulder checks the cell south of it. With room to fall it wobbles (falling=2) for 4
  # ticks, then rolls one cell per tick (falling=1) until blocked. A rolling boulder crushes the miner.
  - Name: fall
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src:
          Object: boulder
          Preconditions: [{{eq: [src.falling, 0]}}]
          Commands: [{{set: [falling, 2]}}, {{set_tile: 1}}, {fall(4)}]
        Dst: {{Object: {FLOOR}}}
      - Src:
          Object: boulder
          Preconditions: [{{gte: [src.falling, 1]}}]
          Commands: [{{mov: _dest}}, {{set: [falling, 1]}}, {{set_tile: 1}}, {fall(1)}]
        Dst: {{Object: {FLOOR}}}
      - Src:
          Object: boulder
          Preconditions: [{{eq: [src.falling, 1]}}]
          Commands: [{fall(2)}]
        Dst: {{Object: {me}, Commands: [{{remove: true}}]}}
      - Src:
          Object: boulder
          Preconditions: [{{neq: [src.falling, 1]}}]
          Commands: [{{set: [falling, 0]}}, {{set_tile: 0}}, {fall(2)}]
        Dst: {{Object: {me}}}
      - Src: {{Object: boulder, Commands: [{{set: [falling, 0]}}, {{set_tile: 0}}, {fall(2)}]}}
        Dst: {{Object: {blocked_fall}}}

  # Lava oozes into neighbouring dug tunnels (never onto natural cave floor) and burns the miner.
  - Name: flow
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: lava, Commands: [{{spawn: lava}}, {flow}]}}
        Dst: {{Object: tunnel}}
      - Src: {{Object: lava, Commands: [{flow}]}}
        Dst: {{Object: {me}, Commands: [{{remove: true}}]}}
      - Src: {{Object: lava, Commands: [{flow}]}}
        Dst: {{Object: {blocked_flow}}}

  # The miner walks up / down / left / right. Walking into loose ore, timber or a ration picks it up,
  # into an empty cart with {CART_ORE} ore fills it, into the lift (all carts full) wins, and into a boulder
  # from the east or west shoves it.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {FLOOR[:-1]}, beam]}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{incr: ore}}, {{reward: 1}}]}}
        Dst: {{Object: ore, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{add: [timber, 2]}}]}}
        Dst: {{Object: timber, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{add: [stamina, {RATION}]}}]}}
        Dst: {{Object: ration, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.ore, {CART_ORE}]}}]
          Commands: [{face}, {{sub: [ore, {CART_ORE}]}}, {{reward: 5}}]
        Dst: {{Object: cart, Commands: [{{decr: carts_needed}}, {{change_to: cart_full}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{lte: [carts_needed, 0]}}]
          Commands: [{face}, {{reward: 10}}]
        Dst: {{Object: lift, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{neq: [meta.facing, 0]}}, {{neq: [meta.facing, 2]}}]
          Commands: [{face}, {{cascade: _dest}}, {{mov: _dest}}]
        Dst: {{Object: boulder}}
      - Src: {{Object: boulder, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: {me}, Commands: [{{remove: true}}]}}
        Dst: {{Object: lava}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: [{SCENERY}, bedrock, dirt_block, rock_block, vein, cart, cart_full, lift]}}

  # Swing the pickaxe at the block in front (the UI sends the last move direction). Costs 1 stamina.
  - Name: dig
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{stam}]
          Commands: [{face}, {{decr: stamina}}, {tunnel}]
        Dst: {{Object: dirt_block, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{stam}, {{gte: [dst.hp, 2]}}]
          Commands: [{face}, {{decr: stamina}}]
        Dst: {{Object: rock_block, Commands: [{{decr: hp}}, {{set_tile: 1}}]}}
      - Src:
          Object: {me}
          Preconditions: [{stam}, {{lte: [dst.hp, 1]}}]
          Commands: [{face}, {{decr: stamina}}, {tunnel}]
        Dst: {{Object: rock_block, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{stam}]
          Commands: [{face}, {{decr: stamina}}, {{incr: ore}}, {{reward: 1}}, {tunnel}]
        Dst: {{Object: vein, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst([o for o in ALL_TOP + C.GROUND_NAMES + ["_empty", "tunnel"] if o != me])}}}

  # Put a support beam into the free cell in front (costs 1 timber).
  - Name: build
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.timber, 1]}}]
          Commands: [{face}, {{decr: timber}}, {{spawn: beam}}]
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst([o for o in ALL_TOP if o != me])}}}
'''


def crack(px):
    dark = (40, 34, 30, 255)
    for x, y in [(12, 15), (13, 16), (14, 17), (14, 18), (15, 19), (16, 20), (17, 20), (18, 21), (16, 21), (15, 22),
                 (20, 22), (21, 23), (9, 24), (10, 25), (11, 26), (11, 27), (23, 18), (22, 19)]:
        I.put(px, x, y, dark)


def crack2d(px):
    dark = (30, 25, 20, 255)
    for x, y in [(6, 4), (7, 5), (8, 6), (8, 7), (9, 8), (10, 9), (11, 10), (12, 10), (13, 11), (12, 12), (11, 13),
                 (15, 12), (16, 13), (17, 14), (5, 15), (6, 16), (7, 17), (8, 18), (18, 6), (17, 7)]:
        I.put(px, x, y, dark)


def objects():
    terrain = I.terrain()
    pre = "remastered/minecrawler"
    grey = f"{I.ISO}/wall-grey-1.png"
    # iso blocks: lowered cubes so tunnels stay visible
    dirt_iso = I.lowered(I.tint(grey, f"{pre}-dirt-full.png", rgb=(1.35, 0.95, 0.6)), f"{pre}-dirt-iso.png", 14)
    rock_full = I.tint(grey, f"{pre}-rock-full.png", rgb=(1.0, 1.0, 1.05))
    rock_iso = I.lowered(rock_full, f"{pre}-rock-iso.png", 10)
    rock_cr_iso = I.overlay(rock_iso, f"{pre}-rock-cracked-iso.png", crack)
    bedrock_iso = I.lowered(I.tint(grey, f"{pre}-bedrock-full.png", rgb=(0.55, 0.5, 0.6)), f"{pre}-bedrock-iso.png", 4)
    vein_iso = I.composite([(rock_iso, 0, 0), (I.fit(f"{I.ISO}/minerals-1-0.png", f"{pre}-vein-gem.png", 32, 48, size=20,
                                                       foot=29), 0, 0)], f"{pre}-vein-iso.png")
    boulder_iso = I.sphere(f"{pre}-boulder-iso.png", (150, 128, 108))
    boulder_wob_iso = I.sphere(f"{pre}-boulder-wobble-iso.png", (176, 120, 90), cx=17)
    tunnel_iso = I.tint(f"{I.ISO}/floor-1.png", f"{pre}-tunnel-iso.png", rgb=(1.0, 0.8, 0.6))
    cave_iso = I.tint(f"{I.ISO}/floor-1.png", f"{pre}-cave-iso.png", rgb=(0.8, 0.8, 0.85))
    beam_iso = I.beam(f"{pre}-beam-iso.png")
    timber_iso = I.billboard("gvgai/newset/log.png", f"{pre}-timber-iso.png", 22, 41)
    ration_iso = I.billboard("sprite2d/food.png", f"{pre}-ration-iso.png", 16, 40)
    cart_iso = I.lowered(f"{I.ISO}/crate-1.png", f"{pre}-cart-iso.png", 2, split=30)
    cart_full_iso = I.composite([(cart_iso, 0, 0), (I.fit(f"{I.ISO}/ore-6.png", f"{pre}-cart-ore.png", 32, 48, size=14,
                                                            foot=26), 0, 0)], f"{pre}-cart-full-iso.png")
    lift_iso = I.tint(f"{I.ISO}/base-1.png", f"{pre}-lift-iso.png", rgb=(1.6, 1.3, 0.7))
    # 2D sprites
    rock2d = "oryx/oryx_fantasy/wall2-0.png"
    rock_cr2d = I.overlay(rock2d, f"{pre}-rock-cracked.png", crack2d)
    vein2d = I.composite([(rock2d, 0, 0), (I.fit("oryx/oryx_fantasy/jewel-0.png", f"{pre}-vein-gem2d.png", size=16), 0, 0)],
                         f"{pre}-vein.png")
    boulder2d = I.sphere(f"{pre}-boulder.png", (150, 128, 108), cx=12, cy=12, r=10, squash=0.95, w=24, h=24, shadow=False)
    boulder_wob2d = I.sphere(f"{pre}-boulder-wobble.png", (176, 120, 90), cx=12, cy=13, r=10, squash=0.95, w=24, h=24,
                             shadow=False)
    beam2d = I.fit(beam_iso, f"{pre}-beam.png", size=22)
    cart_full2d = I.composite([(C.TG + "crate.png", 0, 0), (I.fit("oryx/oryx_fantasy/ore-6.png", f"{pre}-cart-ore2d.png",
                                                                   16, 16, size=10, foot=9), 0, 0)], f"{pre}-cart-full.png")
    avatar2d = C.facing_sprites("oryx/oryx_fantasy/avatars/dwarf1.png", "minecrawler-miner")
    avatar_iso = I.facing_iso(f"{I.ISO}/avatars/gnome-1.png", "minecrawler-miner")
    miner_extra = ("    Variables:\n"
                   f"      - Name: ore\n        InitialValue: 0\n      - Name: stamina\n        InitialValue: {STAMINA}\n"
                   "      - Name: timber\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION)
    boulder_extra = ("    Variables:\n      - Name: falling\n        InitialValue: 0\n"
                     "    InitialActions:\n      - Action: fall\n        ActionId: 1\n        Delay: 2\n")
    lava_extra = "    InitialActions:\n      - Action: flow\n        Delay: 7\n        Randomize: true\n"
    rock_extra = "    Variables:\n      - Name: hp\n        InitialValue: 2\n"
    cart_extra = "    InitialActions:\n      - Action: count_goal\n        ActionId: 1\n"
    out = [
        I.obj("tunnel", "n", 0, [C.TG + "floor_dirt_b.png"], [I.iso_floor(tunnel_iso)], "[0.35, 0.25, 0.15]", "square", 0.9),
        I.obj("bedrock", "W", 1, [C.tiled("oryx/oryx_fantasy/wall4")], [bedrock_iso], "[0.3, 0.3, 0.35]", "square", 1.0),
        I.obj("dirt_block", "d", 1, [C.tiled("gvgai/oryx/dirtWall", "_")], [dirt_iso], "[0.5, 0.35, 0.2]", "square", 1.0),
        I.obj("rock_block", "r", 1, [rock2d, rock_cr2d], [rock_iso, rock_cr_iso], "[0.55, 0.55, 0.6]", "square", 1.0,
              rock_extra, [1.0, 0.8]),
        I.obj("vein", "v", 1, [vein2d], [vein_iso], "[0.3, 0.8, 0.5]", "square", 1.0),
        I.obj("boulder", "O", 1, [boulder2d, boulder_wob2d], [boulder_iso, boulder_wob_iso], "[0.6, 0.5, 0.4]",
              "circle", 0.9, boulder_extra, [0.9, 0.8]),
        I.obj("lava", "L", 1, [C.TG + "lava.png"], [(f"{I.ISO}/lava-1.png", {"TilingMode": "ISO_FLOOR", "Offset": [0, 4]})],
              "[1.0, 0.35, 0.05]", "square", 1.0, lava_extra),
        I.obj("ore", "o", 1, ["oryx/oryx_fantasy/ore-6.png"], [f"{I.ISO}/ore-6.png"], "[0.6, 0.2, 0.7]", "triangle", 0.5),
        I.obj("timber", "t", 1, ["gvgai/newset/log.png"], [timber_iso], "[0.6, 0.4, 0.2]", "square", 0.5),
        I.obj("ration", "f", 1, ["sprite2d/food.png"], [ration_iso], "[0.9, 0.5, 0.4]", "circle", 0.5),
        I.obj("cart", "c", 1, [C.TG + "crate.png"], [cart_iso], "[0.6, 0.45, 0.2]", "square", 0.7, cart_extra),
        I.obj("cart_full", None, 1, [cart_full2d], [cart_full_iso], "[0.6, 0.3, 0.7]", "square", 0.7),
        I.obj("lift", "E", 1, ["gvgai/oryx/stairsup1.png"], [lift_iso], "[1.0, 0.85, 0.2]", "square", 0.9),
        I.obj("beam", None, 2, [beam2d], [beam_iso], "[0.7, 0.5, 0.25]", "square", 0.4),
        I.obj("miner", "A", 1, avatar2d, avatar_iso, "[0.2, 0.6, 1.0]", "triangle", 0.8, miner_extra),
    ]
    return "\nObjects:\n" + terrain + "".join(out), cave_iso


def main():
    objs, cave_iso = objects()
    variables = "    - Name: carts_needed\n      InitialValue: 0\n"
    parts = [I.header("Deep Delve", f"""
        An isometric mine. Dig through dirt and rock (`dig`, two hits for rock), mine ore veins and fill
        every ore cart ({CART_ORE} ore each), then ride the lift out. Digging costs stamina (rations
        restore it). The mine slopes south: undermined boulders wobble, then roll and crush you. Lava
        oozes into dug tunnels. Timber support beams (`build`) stop both.
        """.replace("        ", ""), "miner", ["eq: [lift:count, 0]"], ["eq: [miner:count, 0]"],
        background="dungeon", iso_background=cave_iso, variables=variables)]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid="W")
        info = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9800 + i, stack_under="A")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:16s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objs
    C.emit(GID, gdy, {
        "name": "Deep Delve",
        "tagline": "Dig, mine and dodge cave-ins in an isometric mine.",
        "description": "Tunnel through dirt and rock with your pickaxe, mine ore veins and fill the ore carts "
                       "before taking the lift out. Undermined boulders roll downhill and lava oozes into fresh "
                       "tunnels - prop them up with timber beams.",
        "tags": ["action", "puzzle", "mining", "isometric"],
        "difficulty": "hard",
        "original": "",
        "accent": "#f97316",
        "icon": "⛏️",
        "controls": {"move": "move", "extra": [
            {"key": " ", "label": "Dig", "action": "dig", "directional": True},
            {"key": "b", "label": "Build beam", "action": "build", "directional": True}]},
        "hud": [{"label": "Carts to fill", "kind": "global", "variable": "carts_needed", "tone": "violet"},
                {"label": "Ore", "kind": "var", "object": "miner", "variable": "ore", "tone": "fuchsia"},
                {"label": "Stamina", "kind": "var", "object": "miner", "variable": "stamina", "tone": "emerald"},
                {"label": "Timber", "kind": "var", "object": "miner", "variable": "timber", "tone": "amber"}],
        "win_text": "The lift creaks up into the daylight, carts full of ore!",
        "lose_text": "Buried in the deep.",
    })


if __name__ == "__main__":
    main()
