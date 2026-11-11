#!/usr/bin/env python3
"""Generate the "Spiders Garden" GDY: hand-drawn level layouts + seeded terrain decoration.

Writes resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml (source of truth) and
the demo's bundled copy at studio/griddly-demo/apps/web/src/games/minigrid-spiders-garden.yaml.

Layout characters (one per cell):
  W stone wall   H hedge      ~ water      = bridge     . floor
  b bush         T tree       t tombstone               (obstacles, drawn over the ground)
  A gnome        G spider     g gem
Ground decoration (grass / moss / dirt / bones / mushrooms / skulls) is sprinkled with a seeded RNG
onto walkable cells, so levels look organic but are reproducible. Every level is validated: rectangular,
one gnome, one gem reachable from the gnome, no spider within 3 tiles of the start.
"""
from __future__ import annotations

import collections
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = [
    ROOT / "resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml",
    ROOT / "studio/griddly-demo/apps/web/src/games/minigrid-spiders-garden.yaml",
]

BLOCKING = set("WH~btT")
OBSTACLE_OVER_GROUND = set("btT")  # obstacles that sit on top of ground decoration
ENTITIES = set("AGg")

# name, decoration recipe, layout
LEVELS: list[tuple[str, dict, str]] = [
    ("Pond", dict(grass=4, moss=2, mush=3), """
WWWWWWWWWWWWW
W...........W
W.A..~~~....W
W...~~~~~.G.W
W..b~~~~~...W
W....~~~..b.W
W.G.......g.W
W...........W
WWWWWWWWWWWWW
"""),
    ("Moat", dict(grass=5, moss=3, mush=3, skull=1), """
WWWWWWWWWWWWWWWWW
WWWWW.......WWWWW
WWW.....G.....WWW
WW...~~~~~~~...WW
WW.~~~~~~~~~~~.WW
W..~~~~~~~~~~~..W
WA.~~~~g.===...GW
W..~~~~~~~~~~~..W
WW.~~~~~~~~~~~.WW
WW...~~~~~~~...WW
WWW...........WWW
WWWWW...G...WWWWW
WWWWWWWWWWWWWWWWW
"""),
    ("Hedge cross", dict(grass=7, dirt=2, mush=2), """
HHHHHHHHHHHHHHHHH
HHHHHH.....HHHHHH
HHHHHH..g..HHHHHH
HHHHHH.b.b.HHHHHH
HHHHHH..G..HHHHHH
HHHHHH.....HHHHHH
H......T.T......H
HA.....G.G.....GH
H......T.T......H
HHHHHH.....HHHHHH
HHHHHH..G..HHHHHH
HHHHHH.~~~.HHHHHH
HHHHHHHHHHHHHHHHH
"""),
    ("Lake islands", dict(grass=6, moss=2, mush=2), """
~~~~~~~~~~~~~~~~~~~~~
~.....~~~~~~~~~..b..~
~.A...=========..G..~
~...T.~~~~~~~~~.....~
~~=~~~~~~~~~~~~~~=~~~
~~=~~~~~~~~~~~~~~=~~~
~~=~~~~~~~~~~~~~~=~~~
~.b...~~~~~~~~~...T.~
~..G..=========..g..~
~.....~~~~~~~~~.....~
~~~~~~~~~~~~~~~~~~~~~
"""),
    ("Mossy tunnel", dict(moss=9, bones=4, skull=3, mush=2), """
WWWWWWWWWWWWWWWWWWWWW
WA..............WWWWW
W.......W..W.....WWWW
WWWWWWWWWWWWWW....WWW
WWWWWWWWWWWWWW..G.WWW
WWW..G...W..........W
WW.................WW
WW...WWWWWWWWWWWWWWWW
WW..WWWWWWWWWWWWWWWWW
WW...........W....WWW
WWW...WW...G.....G.WW
WWWWWWWWWWWWWWW....WW
Wg..............G..WW
W.......W.....W....WW
WWWWWWWWWWWWWWWWWWWWW
"""),
    ("Crypt", dict(dirt=6, bones=5, skull=4, moss=2), """
WWWWWWWWWWWWWWW
W..t...A...t..W
WW...........WW
WWW..t...t..WWW
WWWW......GWWWW
WWWWW.....WWWWW
WWWWWW.G.WWWWWW
WWWWWWW.WWWWWWW
WWWWWW...WWWWWW
WWWWW..G..WWWWW
WWWW.t...t.WWWW
WWW...G.....WWW
WW..t.....t..WW
W......g......W
WWWWWWWWWWWWWWW
"""),
    ("Spider pens", dict(grass=8, dirt=3, mush=3), """
HHHHHHHHHHHHHHHHHHHHHHH
HHHHH.HHHHH.HHHHH.HHHHH
HHH.G..bH.G...H.G..bHHH
HHH.....H.....H.....HHH
HHHHH.HHHHH.HHHHH.HHHHH
HA...................gH
HHHHH.HHHHH.HHHHH.HHHHH
HHHb....H.....H.....HHH
HHH...G.H...G.H...G.HHH
HHHHH.HHHHH.HHHHH.HHHHH
HHHHHHHHHHHHHHHHHHHHHHH
"""),
    ("Swamp", dict(grass=6, moss=7, mush=4, bones=2), """
WWWWWWWWWWWWWWWWWWWWW
WA..~~....T....~~...W
W...~~..G.....~~~.G.W
W.......~~~.......T.W
WT..~.....~~..b.....W
W..~~~..G.....~~~...W
W...~~.....T...~~.G.W
W.b.....~~~.........W
W....G..~~~..~~..b..W
W..T.........~~.....W
W.....~~..G......~~gW
W....~~~.........~~.W
WWWWWWWWWWWWWWWWWWWWW
"""),
]


def validate(name: str, rows: list[str]) -> None:
    w = len(rows[0])
    assert all(len(r) == w for r in rows), f"{name}: ragged rows {[len(r) for r in rows]}"
    cells = {(x, y): c for y, r in enumerate(rows) for x, c in enumerate(r)}
    a = [p for p, c in cells.items() if c == "A"]
    g = [p for p, c in cells.items() if c == "g"]
    assert len(a) == 1 and len(g) == 1, f"{name}: need exactly one A and one g"
    seen, q = {a[0]}, collections.deque(a)
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n in cells and cells[n] not in BLOCKING and n not in seen:
                seen.add(n)
                q.append(n)
    assert g[0] in seen, f"{name}: gem unreachable"
    ax, ay = a[0]
    near = [p for p, c in cells.items() if c == "G" and abs(p[0] - ax) + abs(p[1] - ay) <= 3]
    assert not near, f"{name}: spider within 3 of start {near}"


def decorate(rows: list[str], recipe: dict, seed: int) -> list[list[str]]:
    """Return the ground layer: one of '', ',', 'm', 'd', 'o', 'u', 'k' per cell."""
    rng = random.Random(seed)
    h, w = len(rows), len(rows[0])
    ground = [["" for _ in range(w)] for _ in range(h)]
    walkable = [(x, y) for y in range(h) for x in range(w) if rows[y][x] not in "WH~="]

    def blob(char: str, count: int, rmin: float, rmax: float) -> None:
        for _ in range(count):
            cx, cy = rng.choice(walkable)
            r = rng.uniform(rmin, rmax)
            for x, y in walkable:
                d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
                if d <= r and rng.random() < 1.15 - d / (r + 0.01) * 0.6:
                    ground[y][x] = char

    blob("d", recipe.get("dirt", 0), 1.0, 2.5)
    blob("m", recipe.get("moss", 0), 1.0, 2.8)
    blob(",", recipe.get("grass", 0), 1.2, 3.0)
    # grass likes to grow around bushes and trees
    for y in range(h):
        for x in range(w):
            if rows[y][x] in "bT":
                for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if (nx, ny) in walkable and rng.random() < (1 if (dx, dy) == (0, 0) else 0.6):
                        ground[ny][nx] = ","
    # small scattered props on otherwise bare, unoccupied floor
    for char, key in (("u", "mush"), ("k", "skull"), ("o", "bones")):
        free = [(x, y) for x, y in walkable if rows[y][x] == "." and ground[y][x] in ("", "m", "d")]
        for x, y in rng.sample(free, min(recipe.get(key, 0), len(free))):
            ground[y][x] = char
    return ground


def level_tokens(rows: list[str], ground: list[list[str]]) -> str:
    out = []
    for y, r in enumerate(rows):
        toks = []
        for x, c in enumerate(r):
            gch = ground[y][x]
            if c in "WH~=":
                toks.append(c)
            elif c == ".":
                toks.append(gch or ".")
            else:  # entity or obstacle drawn over the ground
                toks.append(f"{gch}/{c}" if gch and gch not in "uko" else c)
        out.append("  ".join(t.ljust(3) for t in toks).rstrip())
    return "\n".join(out)


HEADER = '''Version: "0.1"
Environment:
  Name: Spiders Garden
  Description: |
    Guide the gnome to the gem through gardens, swamps and crypts without touching a spider.
    Spiders wander at random. Water, walls, hedges, bushes, trees and tombstones block everyone;
    grass, moss, dirt and bridges are walkable. Generated by studio/griddly-demo/tools/gen_spiders_garden.py.
  Observers:
    Sprite2D:
      TileSize: 24
      BackgroundTile: oryx/oryx_fantasy/floor3-0.png
      RotateAvatarImage: false
    Block2D:
      TileSize: 24
  Player:
    Observer:
      TrackAvatar: true
      RotateWithAvatar: false
      Height: 7
      Width: 7
    AvatarObject: gnome
  Termination:
    Win:
      - eq: [gem:count, 0]
    Lose:
      - eq: [gnome:count, 0]
  Levels:
'''

WALKABLE = "[grass, moss, dirt, bones, mushroom, skull, bridge]"
SOLID = "[wall, hedge, water, bush, tree, tombstone, gem]"

ACTIONS = f'''
Actions:
  # Spiders wander: rotate left/right or step forwards, at random.
  - Name: random_movement
    InputMapping:
      Inputs:
        1:
          Description: Rotate left
          OrientationVector: [-1, 0]
        2:
          Description: Move forwards
          OrientationVector: [0, -1]
          VectorToDest: [0, -1]
        3:
          Description: Rotate right
          OrientationVector: [1, 0]
      Relative: true
      Internal: true
    Behaviours:
      - Src:
          Object: spider
          Commands:
            - rot: _dir
            - exec: {{Action: random_movement, Delay: 3, Randomize: true}}
        Dst:
          Object: spider
      - Src:
          Object: spider
          Commands:
            - mov: _dest
            - exec: {{Action: random_movement, Delay: 3, Randomize: true}}
        Dst:
          Object: _empty
      - Src:
          Object: spider
          Commands:
            - mov: _dest
            - exec: {{Action: random_movement, Delay: 3, Randomize: true}}
        Dst:
          Object: {WALKABLE}
      - Src:
          Object: spider
          Commands:
            - exec: {{Action: random_movement, Delay: 3, Randomize: true}}
        Dst:
          Object: {SOLID}
      - Src:
          Object: spider
        Dst:
          Object: gnome
          Commands:
            - remove: true
            - reward: -1

  # The gnome moves up / down / left / right (default absolute mapping: 1 left, 2 up, 3 right, 4 down).
  - Name: move
    Behaviours:
      - Src:
          Object: gnome
          Commands:
            - remove: true
            - reward: -1
        Dst:
          Object: spider
      - Src:
          Object: gnome
          Commands:
            - mov: _dest
        Dst:
          Object: _empty
      - Src:
          Object: gnome
          Commands:
            - mov: _dest
        Dst:
          Object: {WALKABLE}
      - Src:
          Object: gnome
          Commands:
            - reward: 1
        Dst:
          Object: gem
          Commands:
            - remove: true
'''


def wall16(prefix: str, sep: str = "-") -> str:
    return "\n".join(f"            - {prefix}{sep}{i}.png" for i in range(16))


def obj(name, char, z, sprite, color, shape="square", scale=1.0, initial=False):
    init = ("    InitialActions:\n      - Action: random_movement\n        Delay: 3\n        Randomize: true\n"
            if initial else "")
    return f'''  - Name: {name}
    MapCharacter: '{char}'
    Z: {z}
{init}    Observers:
      Sprite2D:
{sprite}
      Block2D:
        - Shape: {shape}
          Color: {color}
          Scale: {scale}
'''


def img(path):
    return f"        - Image: {path}"


def tiled(prefix, sep="-"):
    return f"        - TilingMode: WALL_16\n          Image:\n{wall16(prefix, sep)}"


OBJECTS = "\nObjects:\n" + "".join([
    obj("wall", "W", 0, tiled("oryx/oryx_fantasy/wall2"), "[0.45, 0.45, 0.45]"),
    obj("hedge", "H", 0, tiled("oryx/oryx_fantasy/wall9"), "[0.15, 0.35, 0.15]"),
    obj("water", "~", 0, img("oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_water.png"), "[0.1, 0.35, 0.7]"),
    obj("bridge", "=", 0, img("gvgai/newset/logm.png"), "[0.55, 0.4, 0.25]"),
    obj("grass", ",", 0, tiled("gvgai/oryx/grass", "_"), "[0.25, 0.55, 0.25]", scale=0.9),
    obj("moss", "m", 0, img("oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_moss_a.png"), "[0.25, 0.4, 0.2]", scale=0.9),
    obj("dirt", "d", 0, img("oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_dirt_a.png"), "[0.3, 0.22, 0.15]", scale=0.9),
    obj("bones", "o", 0, img("oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_bones_a.png"), "[0.8, 0.8, 0.75]", "circle", 0.3),
    obj("mushroom", "u", 0, img("gvgai/oryx/mushroom2.png"), "[0.8, 0.2, 0.2]", "circle", 0.3),
    obj("skull", "k", 0, img("gvgai/oryx/skull1.png"), "[0.9, 0.9, 0.9]", "circle", 0.3),
    obj("bush", "b", 1, img("gvgai/oryx/bush1.png"), "[0.1, 0.45, 0.15]", "circle", 0.8),
    obj("tree", "T", 1, img("gvgai/oryx/tree2.png"), "[0.05, 0.4, 0.2]", "circle", 0.9),
    obj("tombstone", "t", 1, img("gvgai/oryx/tombstone1.png"), "[0.5, 0.5, 0.5]", "square", 0.7),
    obj("gem", "g", 1, img("oryx/oryx_fantasy/ore-6.png"), "[0.0, 1.0, 0.0]", "triangle", 0.5),
    obj("spider", "G", 2, img("oryx/oryx_fantasy/avatars/spider1.png"), "[1.0, 0.0, 0.0]", "triangle", 0.8, initial=True),
    obj("gnome", "A", 2, img("oryx/oryx_fantasy/avatars/gnome1.png"), "[0.0, 0.0, 1.0]", "triangle", 0.8),
])


def main() -> None:
    parts = [HEADER]
    for i, (name, recipe, layout) in enumerate(LEVELS):
        rows = layout.strip("\n").split("\n")
        validate(name, rows)
        ground = decorate(rows, recipe, seed=1000 + i)
        body = level_tokens(rows, ground)
        parts.append(f"    # {name}\n    - |\n" + "\n".join("      " + r for r in body.split("\n")) + "\n")
        print(f"{i}: {name:13s} {len(rows[0])}x{len(rows)}  spiders={layout.count('G')}", file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + OBJECTS
    for p in OUT:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(gdy)
        print(f"wrote {p.relative_to(ROOT)}", file=sys.stderr)


if __name__ == "__main__":
    main()
