#!/usr/bin/env python3
"""Slime Ranch: an isometric slime farm. Grow, merge and feed slimes, sell their gems, breed kings.

A new game (no original) drawn mainly with ISOMETRIC cameras (Isometric whole map, IsoFollow tracking
the rancher); it also has the usual top-down Sprite2D / FollowCam / CloseUp cameras.

Rules
  * Slimes come in three colours (green, blue, pink) and three tiers (render tiles 0..2: small slime,
    big slime, jelly). They wander slowly inside their pens; pen gates let the rancher through but a
    wandering slime never steps on one (you can still push slimes through a gate).
  * Walking into a slime pushes it (a pushed slime swallows a gem in its way). Pushing a slime into another slime of the SAME colour and the SAME
    tier merges them into one slime of the next tier (set_tile). Two jellies merge into a King slime.
  * Walking into a ripe crop harvests it (carry up to 3 food); it regrows after a while.
  * `feed` (directional, Space) gives 1 food to the slime in front: small/big slimes grow one tier, a
    jelly coughs up a gem at once, a King coughs up two.
  * Jellies also produce a gem every ~30 ticks, Kings every ~12, into a free neighbouring cell. Walk
    into a gem to pick it up.
  * Gem orders: walk into an empty order pedestal while carrying a gem to fill it. Royal thrones count
    how many Kings the level wants. Win: every pedestal filled and enough Kings bred.
  * Wild beasts roam outside the pens (later levels): a beast that bumps into you gobbles your food.

Layout characters (besides the shared terrain chars from common):
  A rancher  g/b/p green/blue/pink slime  C crop  P gem order pedestal  T royal throne  F fence
  G pen gate  H hay crate  B beast  . grass
Every level is validated (rectangular, one rancher, crops/pedestals/thrones/slime pens reachable and
enough slimes of one colour for each King); bot_slimefarm.mjs proves each level winnable in the engine
and records the winning runs (solutions/slimefarm.json).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import isokit as I  # noqa: E402

GID = "slimefarm"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE, DIRT, SAND, MUSH = G["bridge"], G["dirt"], G["sand"], G["mushroom"]
WALK = set(".") | set(G.values()) | {"G"}
COLOURS = {"g": ("green", 0.33), "b": ("blue", 0.58), "p": ("pink", 0.88)}
SLIMES = [f"slime_{n}" for n, _ in COLOURS.values()]
SOLID_MAP = "FHT" + TREE + BUSH + ROCK + HEDGE + STONE + WATER + TORCH + TOMB


# ----------------------------------------------------------------------------- level painting
class Ranch(C.Map):
    def pen(self, x0, y0, x1, y1, gates, slimes=""):
        """Fenced pen (fence on the rectangle border) with gates [(x, y)] and slimes scattered inside."""
        self.rect(x0, y0, x1, y1, "F", filled=False)
        self.rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, ".")
        for p in gates:
            self.set(*p, "G")
        cells = [(x, y) for y in range(y0 + 1, y1) for x in range(x0 + 1, x1)]
        self.rng.shuffle(cells)
        # keep the cells right inside the gates clear
        inside = {(gx + dx, gy + dy) for gx, gy in gates for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))}
        cells = [p for p in cells if p not in inside]
        for ch, p in zip(slimes, cells):
            self.set(*p, ch)

    def field(self, x0, y0, x1, y1, every=1):
        """Crop field: rows of crops on tilled earth with walkable furrows between them."""
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, "C" if (y - y0) % 2 == 0 and (x - x0) % every == 0 else DIRT)


def green_pastures():
    m = Ranch(22, 14, seed=11, border=TREE)
    m.pen(11, 2, 19, 8, [(11, 5)], "gggg")
    m.field(2, 9, 8, 11)
    m.set(6, 4, "A")
    for p in [(2, 6), (4, 6), (6, 6)]:
        m.set(*p, "P")
    m.blob(15, 11.5, 1.6, WATER)
    for p in [(9, 2), (1, 8), (20, 12)]:
        m.set(*p, BUSH)
    m.set(7, 2, "H")
    m.set(8, 2, "H")
    return m, dict(grass=6, mush=4, sand=1)


def two_tone():
    m = Ranch(26, 16, seed=12, border=TREE)
    m.pen(2, 2, 9, 7, [(9, 4)], "ggggg")
    m.pen(16, 2, 23, 7, [(16, 4)], "bbbbb")
    m.field(5, 11, 20, 13, every=1)
    for x in range(11, 15):
        m.set(x, 3, STONE)
    m.set(12, 5, "A")
    for p in [(11, 7), (12, 8), (13, 8), (14, 7)]:
        m.set(*p, "P")
    m.set(2, 10, "H")
    m.set(23, 10, "H")
    m.set(24, 10, "H")
    for p in [(1, 13), (24, 13), (12, 1)]:
        m.set(*p, BUSH)
    return m, dict(grass=10, mush=5, dirt=2)


def royal_pen():
    m = Ranch(26, 17, seed=13, border=HEDGE)
    m.pen(13, 2, 23, 9, [(13, 5), (18, 9)], "pppppp")
    m.field(2, 11, 10, 15)
    m.set(4, 3, "A")
    m.set(4, 7, "T")
    m.set(7, 7, "P")
    m.set(8, 7, "P")
    for p in [(2, 2), (2, 7), (10, 2)]:
        m.set(*p, TORCH)
    m.blob(18, 13.5, 2.0, WATER)
    m.set(14, 13, BRIDGE)
    m.set(23, 12, "H")
    m.set(23, 13, "H")
    return m, dict(grass=8, mush=5, sand=2, bones=2)


def beast_prairie():
    m = Ranch(30, 19, seed=14, border=TREE)
    m.pen(2, 2, 9, 8, [(9, 5)], "gggggg")
    m.pen(20, 2, 27, 8, [(20, 5)], "bbbbbb")
    m.pen(20, 11, 27, 16, [(20, 13)], "ppppp")
    m.field(3, 12, 14, 16, every=2)
    m.set(14, 5, "A")
    m.set(13, 2, "T")
    for p in [(12, 8), (14, 9), (16, 8), (14, 7)]:
        m.set(*p, "P")
    for p in [(16, 2), (11, 2), (17, 15), (16, 11)]:
        m.set(*p, "H" if p[1] < 5 else BUSH)
    m.blob(4.5, 10, 1.0, WATER)
    m.set(11, 13, "B")
    m.set(17, 13, "B")
    return m, dict(grass=14, mush=6, dirt=3, sand=2)


def slime_kingdom():
    m = Ranch(32, 20, seed=15, border=TREE)
    m.pen(2, 2, 10, 8, [(10, 5), (6, 8)], "ggggggg")
    m.pen(21, 2, 29, 8, [(21, 5)], "bbbbbb")
    m.pen(21, 11, 29, 17, [(21, 14), (25, 11)], "ppppppp")
    m.field(2, 12, 13, 17, every=1)
    m.set(15, 6, "A")
    for p in [(14, 2), (17, 2)]:
        m.set(*p, "T")
    for p in [(13, 9), (15, 10), (17, 10), (19, 9)]:
        m.set(*p, "P")
    for x in range(14, 18):
        m.set(x, 14, STONE if x in (14, 17) else ".")
    m.set(15, 16, TORCH)
    m.set(16, 16, TORCH)
    for p in [(12, 2), (19, 2), (1, 10), (30, 10)]:
        m.set(*p, "H" if p[1] == 2 else BUSH)
    m.blob(17, 18, 1.2, WATER)
    m.set(13, 10, "B")
    m.set(18, 12, "B")
    return m, dict(grass=16, mush=8, dirt=3, bones=2)


LEVELS = [
    ("Green Pastures", "Grow your first slimes and fill three gem orders.", green_pastures),
    ("Two-Tone Ranch", "Green and blue pens, four gem orders.", two_tone),
    ("Royal Pen", "Merge pink slimes all the way up to a King.", royal_pen),
    ("Beast Prairie", "Three pens, a throne and beasts that steal your crops.", beast_prairie),
    ("Slime Kingdom", "Two Kings and four gem orders. Rule the ranch!", slime_kingdom),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one rancher"
    passable = lambda c, p: c in WALK or c in "AgbpB"
    seen = C.reach(rows, a[0], passable)
    adj = lambda p: any((p[0] + dx, p[1] + dy) in seen for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    for ch in "CPT":
        bad = [p for p in C.find(rows, ch) if not adj(p)]
        assert not bad, f"{name}: unreachable {ch} at {bad}"
    slimes = {ch: [p for p in C.find(rows, ch)] for ch in COLOURS}
    for ch, ps in slimes.items():
        assert all(p in seen for p in ps), f"{name}: slime {ch} sealed in"
    thrones = len(C.find(rows, "T"))
    peds = len(C.find(rows, "P"))
    assert thrones + peds > 0, f"{name}: nothing to do"
    # each King needs two slimes of one colour; different Kings may share a colour only with 4 slimes
    pairs = sum(len(ps) // 2 for ps in slimes.values())
    assert pairs >= thrones, f"{name}: not enough slime pairs for {thrones} kings"
    assert len(C.find(rows, "C")) >= 3, f"{name}: too few crops"
    return dict(pedestals=peds, thrones=thrones, crops=len(C.find(rows, "C")),
                slimes={COLOURS[k][0]: len(v) for k, v in slimes.items()}, beasts=len(C.find(rows, "B")))


# ----------------------------------------------------------------------------- GDY
SCENERY = ", ".join(C.SCENERY_NAMES)
FLOOR = C.EMPTY_OR_GROUND
ALL = (["_empty"] + C.GROUND_NAMES + C.SCENERY_NAMES +
       ["fence", "gate", "crate", "crop", "pedestal", "pedestal_full", "throne", "gem", "king", "beast", "rancher"]
       + SLIMES)


def lst(names):
    return "[" + ", ".join(names) + "]"


def actions():
    me = "rancher"
    face = "{set_tile: meta.facing}"
    floor_gate = FLOOR[:-1] + ", gate]"
    others_of = lambda names: lst([o for o in ALL if o not in names and o not in C.GROUND_NAMES and o != "_empty"])
    wander_ex = "{exec: {Action: wander, Delay: 8, Randomize: true}}"
    beast_ex = "{exec: {Action: prowl, Delay: 3, Randomize: true}}"
    slime_moves = ""
    for s in SLIMES:
        slime_moves += f'''      - Src: {{Object: {s}, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {floor_gate}}}
      - Src: {{Object: {s}, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: gem, Commands: [{{remove: true}}]}}
      - Src:
          Object: {s}
          Preconditions: [{{eq: [src.tier, dst.tier]}}, {{lt: [src.tier, 2]}}]
          Commands: [{{remove: true}}]
        Dst: {{Object: {s}, Commands: [{{incr: tier}}, {{set_tile: tier}}, {{reward: 2}}]}}
      - Src:
          Object: {s}
          Preconditions: [{{eq: [src.tier, 2]}}, {{eq: [dst.tier, 2]}}]
          Commands: [{{remove: true}}]
        Dst: {{Object: {s}, Commands: [{{decr: kings_needed}}, {{reward: 10}}, {{change_to: king}}]}}
'''
    return f'''
Actions:
{C.face_action(me)}
  # Order pedestals and thrones count themselves into the level's goals at reset.
  - Name: count_goal
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: pedestal, Commands: [{{incr: gems_needed}}]}}
        Dst: {{Object: pedestal}}
      - Src: {{Object: throne, Commands: [{{incr: kings_needed}}]}}
        Dst: {{Object: throne}}

  # Crops ripen (render tile 1) some time after being harvested.
  - Name: ripen
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Ripen, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: crop, Commands: [{{set: [ripe, 1]}}, {{set_tile: 1}}]}}
        Dst: {{Object: crop}}

  # Slimes wander slowly (idle inputs make half the tries a pause) and never step onto gates.
  - Name: wander
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
        5: {{Description: Rest, VectorToDest: [0, 0]}}
        6: {{Description: Rest, VectorToDest: [0, 0]}}
        7: {{Description: Rest, VectorToDest: [0, 0]}}
        8: {{Description: Rest, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {lst(SLIMES)}, Commands: [{{mov: _dest}}, {wander_ex}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: {lst(SLIMES)}, Commands: [{wander_ex}]}}
        Dst: {{Object: {others_of([])}}}

  # Beasts prowl outside the pens; bumping into the rancher they gobble its food.
  - Name: prowl
    InputMapping:
      Internal: true
    Behaviours:
      - Src: {{Object: beast, Commands: [{{mov: _dest}}, {beast_ex}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: beast, Commands: [{beast_ex}]}}
        Dst: {{Object: {others_of([me])}}}
      - Src: {{Object: beast, Commands: [{beast_ex}]}}
        Dst: {{Object: {me}, Commands: [{{set: [food, 0]}}]}}

  # Jellies (tier 2) produce a gem every ~30 ticks into a random free neighbour, Kings every ~12.
  - Name: produce
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src:
          Object: {lst(SLIMES)}
          Preconditions: [{{eq: [src.tier, 2]}}]
          Commands: [{{spawn: gem}}, {{exec: {{Action: produce, Delay: 30, Randomize: true}}}}]
        Dst: {{Object: {FLOOR}}}
      - Src:
          Object: {lst(SLIMES)}
          Preconditions: [{{eq: [src.tier, 2]}}]
          Commands: [{{exec: {{Action: produce, Delay: 4, Randomize: true}}}}]
        Dst: {{Object: {others_of([])}}}
      - Src:
          Object: {lst(SLIMES)}
          Preconditions: [{{lt: [src.tier, 2]}}]
          Commands: [{{exec: {{Action: produce, Delay: 30, Randomize: true}}}}]
        Dst: {{Object: {lst(ALL)}}}
      - Src: {{Object: king, Commands: [{{spawn: gem}}, {{exec: {{Action: produce, Delay: 12, Randomize: true}}}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: king, Commands: [{{exec: {{Action: produce, Delay: 3, Randomize: true}}}}]}}
        Dst: {{Object: {others_of([])}}}

  # The rancher walks up / down / left / right. Walking into a slime pushes it (merging it into a
  # same-colour same-tier slime behind it), into a ripe crop harvests it, into a gem picks it up and
  # into an empty pedestal (carrying a gem) fills the order.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {floor_gate}}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{incr: gems}}, {{reward: 1}}]}}
        Dst: {{Object: gem, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}, {{cascade: _dest}}, {{mov: _dest}}]}}
        Dst: {{Object: {lst(SLIMES)}}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [dst.ripe, 1]}}, {{lt: [src.food, 3]}}]
          Commands: [{face}, {{incr: food}}, {{reward: 1}}]
        Dst: {{Object: crop, Commands: [{{set: [ripe, 0]}}, {{set_tile: 0}}, {{exec: {{Action: ripen, ActionId: 1, Delay: 40}}}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.gems, 1]}}]
          Commands: [{face}, {{decr: gems}}, {{reward: 5}}]
        Dst: {{Object: pedestal, Commands: [{{decr: gems_needed}}, {{change_to: pedestal_full}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: [{SCENERY}, fence, crate, crop, pedestal, pedestal_full, throne, king, beast]}}
{slime_moves}
  # Give one food to the slime in front (directional; the UI sends the last move direction).
  - Name: feed
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.food, 1]}}, {{lt: [dst.tier, 2]}}]
          Commands: [{face}, {{decr: food}}, {{reward: 1}}]
        Dst: {{Object: {lst(SLIMES)}, Commands: [{{incr: tier}}, {{set_tile: tier}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.food, 1]}}, {{eq: [dst.tier, 2]}}]
          Commands: [{face}, {{decr: food}}, {{incr: gems}}, {{reward: 1}}]
        Dst: {{Object: {lst(SLIMES)}}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.food, 1]}}]
          Commands: [{face}, {{decr: food}}, {{add: [gems, 2]}}, {{reward: 2}}]
        Dst: {{Object: king}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst([o for o in ALL if o != me])}}}
'''


def slime_sprites(ch):
    name, hue = COLOURS[ch]
    src2d = {"g": "gvgai/oryx/slime2.png", "b": "gvgai/oryx/slime1.png", "p": "gvgai/oryx/slime3.png"}[ch]
    s2 = [I.fit(src2d, f"remastered/slimefarm-{name}-0.png", size=13, foot=21),
          I.fit(src2d, f"remastered/slimefarm-{name}-1.png", size=19, foot=22),
          I.fit(C.TG.replace("tg_world/tg_world_", "tg_monsters/tg_monsters_") + "jelly_d1.png",
                f"remastered/slimefarm-{name}-2.png", size=23, foot=24, hue=hue, sat=1.2, val=1.3)]
    jelly = f"{I.ISO}/jelly-1.png"
    iso = [I.fit(jelly, f"remastered/slimefarm-{name}-iso-0.png", 32, 48, size=13, foot=42, hue=hue, sat=1.1, val=1.3),
           I.fit(jelly, f"remastered/slimefarm-{name}-iso-1.png", 32, 48, size=19, foot=43, hue=hue, sat=1.1, val=1.3),
           I.fit(jelly, f"remastered/slimefarm-{name}-iso-2.png", 32, 48, size=26, foot=44, hue=hue, sat=1.25, val=1.5)]
    return s2, iso


def crown(px, x0, y0, s=1):
    gold, dark = (250, 204, 21, 255), (120, 80, 10, 255)
    pattern = ["#.#.#", "#####", "#####"]
    for j, row in enumerate(pattern):
        for i, c in enumerate(row):
            if c == "#":
                for a in range(s):
                    for b in range(s):
                        I.put(px, x0 + i * s + b, y0 + j * s + a, gold)
    for i in range(5 * s):
        I.put(px, x0 + i, y0 + 3 * s, dark)


def objects():
    terrain = I.terrain()  # writes the shared iso terrain images (iso-dirt etc.) used below
    avatar2d = C.facing_sprites("oryx/oryx_fantasy/avatars/gnome1.png", "slimefarm-rancher")
    avatar_iso = I.facing_iso(f"{I.ISO}/avatars/gnome-1.png", "slimefarm-rancher")
    goal_init = "    InitialActions:\n      - Action: count_goal\n        ActionId: 1\n"
    slime_extra = ("    Variables:\n      - Name: tier\n        InitialValue: 0\n"
                   "    InitialActions:\n      - Action: wander\n        Delay: 8\n        Randomize: true\n"
                   "      - Action: produce\n        Delay: 30\n        Randomize: true\n")
    king_extra = "    InitialActions:\n      - Action: produce\n        Delay: 6\n        Randomize: true\n"
    crop_extra = "    Variables:\n      - Name: ripe\n        InitialValue: 1\n    InitialActions:\n      - Action: ripen\n        ActionId: 1\n"
    rancher_extra = ("    Variables:\n      - Name: food\n        InitialValue: 0\n      - Name: gems\n        InitialValue: 0\n"
                     + C.FACE_INITIAL_ACTION)
    # sprites
    dirt_iso = "remastered/iso-dirt.png"
    crop2d = [I.fit("sprite2d/sapling.png", "remastered/slimefarm-sprout.png", size=16, foot=21),
              I.fit("sprite2d/plant-ripe.png", "remastered/slimefarm-ripe.png", size=20, foot=22)]
    crop_iso = [I.composite([(dirt_iso, 0, 0), (I.billboard("sprite2d/sapling.png", "remastered/slimefarm-sprout-bb.png", 14, 40), 0, 0)],
                            "remastered/slimefarm-sprout-iso.png"),
                I.composite([(dirt_iso, 0, 0), (I.billboard("sprite2d/plant-ripe.png", "remastered/slimefarm-ripe-bb.png", 22, 41), 0, 0)],
                            "remastered/slimefarm-ripe-iso.png")]
    gem2d = I.fit("oryx/oryx_fantasy/jewel-0.png", "remastered/slimefarm-gem.png", size=14, foot=20, hue=0.88, sat=1.1, val=1.2)
    gem_iso = I.fit(f"{I.ISO}/minerals-1-1.png", "remastered/slimefarm-gem-iso.png", 32, 48, size=16, foot=41,
                    hue=0.88, sat=1.1, val=1.3)
    ped2d = I.composite([(C.TG + "pad_red.png", 0, 0)], "remastered/slimefarm-pedestal.png")
    gem_small = I.fit("oryx/oryx_fantasy/jewel-0.png", "remastered/slimefarm-gem-small.png", 16, 16, size=10, foot=13,
                      hue=0.88, sat=1.1, val=1.2)
    ped_full2d = I.composite([(C.TG + "pad_green.png", 0, 0), (gem_small, 0, 0)], "remastered/slimefarm-pedestal-full.png")
    ped_iso = I.lowered(f"{I.ISO}/base-1.png", "remastered/slimefarm-pedestal-iso.png", 6, split=24)
    ped_full_iso = I.composite([(ped_iso, 0, 0), (I.fit(f"{I.ISO}/minerals-1-1.png", "remastered/slimefarm-gem-iso-sm.png",
                                                          32, 48, size=14, foot=24, hue=0.88, sat=1.1, val=1.4), 0, 0)],
                               "remastered/slimefarm-pedestal-full-iso.png")
    king2d = I.fit("gvgai/oryx/slime6.png", "remastered/slimefarm-king-base.png", size=21, foot=23, hue=0.13, sat=1.2, val=1.3)
    king2d = I.overlay(king2d, "remastered/slimefarm-king.png", lambda px: crown(px, 7, 1, 2))
    king_iso = I.fit(f"{I.ISO}/queen-1.png", "remastered/slimefarm-king-iso.png", 32, 48, size=30, foot=44)
    throne_iso = I.composite([(f"{I.ISO}/barracks-1.png", 0, 0)], "remastered/slimefarm-throne-iso.png")
    throne_iso = I.overlay(throne_iso, "remastered/slimefarm-throne-iso.png", lambda px: crown(px, 11, 11, 2))
    fence_iso = I.billboard("sprite2d/fence.png", "remastered/slimefarm-fence-iso.png", 26, 42)
    gate2d = I.tint("sprite2d/fence.png", "remastered/slimefarm-gate.png", rgb=(1.3, 1.15, 0.6))
    gate_iso = I.tint(f"{I.ISO}/floor-1.png", "remastered/slimefarm-gate-iso.png", rgb=(1.5, 1.05, 0.55))
    beast_iso = f"{I.ISO}/beast-1.png"
    out = [
        I.obj("fence", "F", 1, ["sprite2d/fence.png"], [fence_iso], "[0.55, 0.4, 0.2]", "square", 0.9),
        I.obj("gate", "G", 0, [gate2d], [I.iso_floor(gate_iso)], "[0.8, 0.65, 0.3]", "square", 0.6),
        I.obj("crate", "H", 1, [C.TG + "crate.png"], [f"{I.ISO}/crate-1.png"], "[0.6, 0.45, 0.2]", "square", 0.8),
        I.obj("crop", "C", 1, crop2d, crop_iso, "[0.3, 0.6, 0.1]", "triangle", 0.6, crop_extra, [0.4, 0.8]),
        I.obj("pedestal", "P", 1, [ped2d], [ped_iso], "[0.8, 0.2, 0.5]", "square", 0.6, goal_init),
        I.obj("pedestal_full", None, 1, [ped_full2d], [ped_full_iso], "[0.2, 0.8, 0.5]", "square", 0.6),
        I.obj("throne", "T", 1, [C.TG + "command_chair.png"], [throne_iso], "[0.9, 0.75, 0.1]", "square", 0.8, goal_init),
        I.obj("gem", "x", 1, [gem2d], [gem_iso], "[1.0, 0.3, 0.8]", "triangle", 0.5),
        I.obj("king", "K", 1, [king2d], [king_iso], "[1.0, 0.85, 0.1]", "circle", 1.0, king_extra),
        I.obj("beast", "B", 1, ["gvgai/oryx/bear2.png"], [beast_iso], "[0.6, 0.35, 0.1]", "triangle", 0.8,
              "    InitialActions:\n      - Action: prowl\n        Delay: 3\n        Randomize: true\n"),
    ]
    blocks = {"g": "[0.2, 0.8, 0.3]", "b": "[0.2, 0.5, 0.9]", "p": "[0.9, 0.4, 0.7]"}
    for ch, (name, _) in COLOURS.items():
        s2, iso = slime_sprites(ch)
        out.append(I.obj(f"slime_{name}", ch, 1, s2, iso, blocks[ch], "circle", 0.6, slime_extra, [0.45, 0.65, 0.9]))
    out.append(I.obj("rancher", "A", 1, avatar2d, avatar_iso, "[0.9, 0.9, 0.2]", "triangle", 0.8, rancher_extra))
    return "\nObjects:\n" + terrain + "".join(out)


def main():
    variables = "    - Name: gems_needed\n      InitialValue: 0\n    - Name: kings_needed\n      InitialValue: 0\n"
    parts = [I.header("Slime Ranch", """
        An isometric slime ranch. Push two slimes of the same colour and size together to merge them
        into a bigger slime; two jellies make a King. Harvest crops and `feed` slimes to grow them, or
        feed a jelly for a gem. Jellies and Kings also produce gems over time. Fill every gem order
        pedestal and breed as many Kings as there are thrones. Beasts steal the food you carry.
        """.replace("        ", ""), "rancher",
        ["and: [{lte: [gems_needed, 0]}, {lte: [kings_needed, 0]}]"], [],
        background="gvgai/oryx/grass_15.png", iso_background=f"{I.ISO}/grass-1.png", variables=variables)]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, recipe = build()
        rows = m.rows(solid="F" + TREE + HEDGE)
        info = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9700 + i, stack_under="AgbpB")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:15s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Slime Ranch",
        "tagline": "Merge, feed and breed slimes on an isometric ranch.",
        "description": "Herd wobbly slimes around their pens: push two of the same colour and size together to "
                       "merge them, harvest crops to feed them, collect the gems the jellies produce to fill "
                       "orders, and breed royal King slimes for the thrones.",
        "tags": ["puzzle", "farming", "isometric"],
        "difficulty": "medium",
        "original": "",
        "accent": "#a3e635",
        "icon": "🟢",
        "controls": {"move": "move", "extra": [{"key": " ", "label": "Feed", "action": "feed", "directional": True}]},
        "hud": [{"label": "Gem orders", "kind": "global", "variable": "gems_needed", "tone": "fuchsia"},
                {"label": "Kings wanted", "kind": "global", "variable": "kings_needed", "tone": "amber"},
                {"label": "Food", "kind": "var", "object": "rancher", "variable": "food", "tone": "lime"},
                {"label": "Gems", "kind": "var", "object": "rancher", "variable": "gems", "tone": "violet"}],
        "win_text": "Every order filled. The slimes wobble with pride!",
        "lose_text": "The ranch went quiet.",
    })
    print("images:", len(I.written()), file=sys.stderr)


if __name__ == "__main__":
    main()
