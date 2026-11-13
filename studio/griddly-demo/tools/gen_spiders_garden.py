#!/usr/bin/env python3
"""Generate the "Spiders Garden" GDY: big, themed, seeded levels built from terrain primitives.

Writes resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml (source of truth) and
the demo's bundled copy at studio/griddly-demo/apps/web/src/games/minigrid-spiders-garden.yaml.

Structure characters (layout layer):
  W stone wall   H hedge   R cave rock   ~ water   = bridge   D door (needs a key)
  b bush   T tree   t tombstone   i torch   N spider nest            (solid props)
  A gnome   G spider   P hunter (A* chaser)   B brood nest (hatches hunters)   g gem   K key   . floor
Ground decoration (on walkable cells):  , grass  m moss  d dirt  o bones  u mushroom  x skull

Every level is validated: rectangular, one gnome, every gem/key reachable (doors opened by keys
that are themselves reachable), and no spider or nest within 4 tiles of the start.
"""
from __future__ import annotations

import collections
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).resolve().parent / "remaster"))
import action_kit as K  # noqa: E402
OUT = [
    ROOT / "resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml",
    ROOT / "studio/griddly-demo/apps/web/src/games/minigrid-spiders-garden.yaml",
]

STRUCTURE = set("WHR~=D")
SOLID_PROPS = set("bTtiNB")
BLOCKING = STRUCTURE - {"="} | SOLID_PROPS  # for the gnome, before doors are opened


# ----------------------------------------------------------------------------- grid primitives
class Map:
    def __init__(self, w: int, h: int, seed: int, fill: str = ".", border: str = "W"):
        self.w, self.h, self.rng = w, h, random.Random(seed)
        self.g = [[fill] * w for _ in range(h)]
        self.rect(0, 0, w - 1, h - 1, border, filled=False)

    def inside(self, x, y, margin=1):
        return margin <= x < self.w - margin and margin <= y < self.h - margin

    def get(self, x, y):
        return self.g[y][x]

    def set(self, x, y, c, margin=1):
        if self.inside(x, y, margin):
            self.g[y][x] = c

    def rect(self, x0, y0, x1, y1, c, filled=True):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if filled or x in (x0, x1) or y in (y0, y1):
                    self.set(x, y, c, margin=0)

    def blob(self, cx, cy, r, c, jitter=0.35, only=None):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if not self.inside(x, y):
                    continue
                d = ((x - cx) ** 2 + ((y - cy) * 1.15) ** 2) ** 0.5
                if d <= r * (1 - jitter / 2 + self.rng.random() * jitter) and (only is None or self.g[y][x] in only):
                    self.g[y][x] = c

    def walk(self, p0, p1, c, width=1, wobble=0.35, only=None):
        """Biased random walk from p0 to p1 painting a brush of `width` radius."""
        x, y = p0
        tx, ty = p1
        for _ in range(4 * (self.w + self.h)):
            for dy in range(-width + 1, width):
                for dx in range(-width + 1, width):
                    if self.inside(x + dx, y + dy) and (only is None or self.g[y + dy][x + dx] in only):
                        self.g[y + dy][x + dx] = c
            if (x, y) == (tx, ty):
                return
            if self.rng.random() < wobble:
                x += self.rng.choice((-1, 0, 1))
                y += self.rng.choice((-1, 0, 1))
            elif abs(tx - x) > abs(ty - y) or (abs(tx - x) == abs(ty - y) and self.rng.random() < 0.5):
                x += (tx > x) - (tx < x)
            else:
                y += (ty > y) - (ty < y)
            x = min(max(x, 1), self.w - 2)
            y = min(max(y, 1), self.h - 2)

    def maze(self, x0, y0, x1, y1, wall="H", braid=0.5):
        """Perfect maze (corridor width 1) in the box, then braid dead ends into loops."""
        self.rect(x0, y0, x1, y1, wall)
        cells = [(x, y) for y in range(y0 + 1, y1, 2) for x in range(x0 + 1, x1, 2)]
        start = self.rng.choice(cells)
        seen, stack = {start}, [start]
        self.g[start[1]][start[0]] = "."
        while stack:
            x, y = stack[-1]
            nbrs = [(x + dx, y + dy) for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))
                    if (x + dx, y + dy) in set(cells) and (x + dx, y + dy) not in seen]
            if not nbrs:
                stack.pop()
                continue
            nx, ny = self.rng.choice(nbrs)
            self.g[(y + ny) // 2][(x + nx) // 2] = "."
            self.g[ny][nx] = "."
            seen.add((nx, ny))
            stack.append((nx, ny))
        for x, y in cells:  # braid
            walls = [(dx, dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                     if self.g[y + dy][x + dx] == wall and x0 < x + 2 * dx < x1 and y0 < y + 2 * dy < y1]
            if len(walls) >= 3 and self.rng.random() < braid:
                dx, dy = self.rng.choice(walls)
                self.g[y + dy][x + dx] = "."

    def free(self, region=None, ch="."):
        x0, y0, x1, y1 = region or (1, 1, self.w - 2, self.h - 2)
        return [(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if self.g[y][x] == ch]

    def scatter(self, c, n, region=None, away_from=None, min_dist=0, clear=0):
        """Place n of c on free floor, optionally away from a point and with `clear` free neighbours."""
        spots = self.free(region)
        self.rng.shuffle(spots)
        placed = 0
        for x, y in spots:
            if placed >= n:
                break
            if away_from and abs(x - away_from[0]) + abs(y - away_from[1]) < min_dist:
                continue
            if clear and sum(self.g[y + dy][x + dx] == "." for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))) < clear:
                continue
            self.g[y][x] = c
            placed += 1

    def rows(self):
        """Rows, cropped so at most one fully-solid ring surrounds the playable area."""
        g = self.g
        solid = lambda c: c in STRUCTURE - {"="}
        top = next(y for y in range(self.h) if not all(solid(c) for c in g[y]))
        bot = next(y for y in reversed(range(self.h)) if not all(solid(c) for c in g[y]))
        left = next(x for x in range(self.w) if not all(solid(g[y][x]) for y in range(self.h)))
        right = next(x for x in reversed(range(self.w)) if not all(solid(g[y][x]) for y in range(self.h)))
        y0, y1, x0, x1 = max(top - 1, 0), min(bot + 1, self.h - 1), max(left - 1, 0), min(right + 1, self.w - 1)
        self.w, self.h = x1 - x0 + 1, y1 - y0 + 1
        return ["".join(r[x0:x1 + 1]) for r in g[y0:y1 + 1]]


# ----------------------------------------------------------------------------- levels
def garden_gate():
    m = Map(26, 16, seed=11, border="H")
    m.blob(7, 10, 3.2, "~")                       # pond
    m.set(7, 10, "=")
    m.rect(16, 3, 23, 11, "W", filled=False)      # walled garden
    m.set(16, 7, "D")
    m.scatter("t", 2, (18, 5, 22, 9))
    for p in [(3, 3), (11, 3), (12, 13), (3, 13), (13, 7)]:
        m.set(*p, "T")
    m.scatter("b", 6, (2, 2, 14, 14))
    m.set(2, 2, "A")
    m.set(21, 9, "g")
    m.set(12, 12, "g")
    m.set(3, 12, "K")
    m.scatter("G", 3, (8, 2, 15, 14), away_from=(2, 2), min_dist=7)
    return m, dict(grass=10, moss=2, mush=4, dirt=1)


def riverlands():
    m = Map(36, 20, seed=22, border="T")
    m.walk((17, 0), (20, 19), "~", width=2, wobble=0.45, only=".")
    for y in (4, 10, 16):                          # bridges over the river
        xs = [x for x in range(1, 35) if m.get(x, y) == "~"]
        for x in xs:
            m.set(x, y, "=")
    for cx, cy, r in [(6, 5, 2.2), (28, 14, 2.5), (31, 4, 1.6)]:
        m.blob(cx, cy, r, "T", only=".")
    m.blob(9, 14, 2.4, "~", only=".")
    m.scatter("b", 14)
    m.set(2, 17, "A")
    for p in [(33, 2), (27, 9), (6, 2), (32, 17)]:
        m.set(*p, "g")
    m.scatter("N", 2, (24, 3, 33, 16), clear=3)
    m.scatter("G", 4, away_from=(2, 17), min_dist=10)
    return m, dict(grass=18, moss=3, mush=6, dirt=3)


def hedge_maze():
    m = Map(33, 21, seed=33, border="H")
    m.maze(0, 0, 32, 20, "H", braid=0.75)
    m.rect(12, 7, 20, 13, ".")                     # courtyard
    m.rect(14, 9, 18, 11, "~")                     # fountain
    m.set(16, 9, "=")
    m.set(16, 8, "g")
    for p in [(12, 7), (20, 7), (12, 13), (20, 13)]:
        m.set(*p, "T")
    m.set(1, 1, "A")
    for p in [(31, 1), (1, 19), (31, 19)]:
        m.set(*p, "g")
    m.scatter("G", 6, away_from=(1, 1), min_dist=12)
    m.scatter("P", 1, away_from=(1, 1), min_dist=20)
    return m, dict(grass=14, dirt=4, mush=5)


def sunken_crypt():
    m = Map(34, 22, seed=44, fill="W")
    rooms = [(2, 2, 9, 8), (13, 2, 20, 8), (24, 2, 31, 8), (2, 13, 9, 19), (13, 13, 20, 19), (24, 13, 31, 19)]
    for r in rooms:
        m.rect(*r, ".")
    for (x0, y0, x1, y1), (a0, b0, a1, b1) in [(rooms[0], rooms[1]), (rooms[1], rooms[2]), (rooms[3], rooms[4]),
                                               (rooms[0], rooms[3]), (rooms[1], rooms[4])]:
        m.walk(((x0 + x1) // 2, (y0 + y1) // 2), ((a0 + a1) // 2, (b0 + b1) // 2), ".", width=1, wobble=0.0, only="W.")
    m.walk((20, 16), (24, 16), ".", width=1, wobble=0.0)          # treasure room entrance...
    m.rect(21, 15, 23, 17, "W")
    m.set(21, 16, ".")
    m.set(22, 16, "D")                                             # ...behind a door
    m.set(23, 16, ".")
    m.walk((27, 8), (27, 13), "W", width=1, wobble=0.0)            # seal treasure room from above
    for x0, y0, x1, y1 in rooms[:5]:
        for x in range(x0 + 1, x1, 3):
            m.set(x, y0 + 1, "t") if m.rng.random() < 0.7 else None
        m.set(x0, y0, "i")
        m.set(x1, y0, "i")
    m.set(3, 4, "A")
    m.set(30, 3, "K")
    for p in [(28, 18), (25, 14), (16, 18)]:
        m.set(*p, "g")
    m.scatter("N", 2, (13, 2, 31, 8), clear=3)
    m.scatter("G", 5, away_from=(3, 4), min_dist=12)
    return m, dict(dirt=10, bones=12, skull=8, moss=6)


def bog():
    m = Map(36, 22, seed=55, border="~")
    for _ in range(14):
        m.blob(m.rng.randint(3, 32), m.rng.randint(3, 18), m.rng.uniform(1.0, 2.6), "~", only=".")
    for _ in range(18):
        m.set(m.rng.randint(2, 33), m.rng.randint(2, 19), "T") if m.rng.random() < 0.9 else None
    m.scatter("b", 12)
    m.rect(1, 1, 4, 4, ".")
    m.set(2, 2, "A")
    for p in [(33, 2), (32, 18), (18, 10), (5, 18)]:
        m.rect(p[0] - 1, p[1] - 1, p[0] + 1, p[1] + 1, ".")
        m.set(*p, "g")
    m.scatter("N", 1, (14, 6, 30, 16), clear=4)
    m.scatter("G", 7, away_from=(2, 2), min_dist=10)
    return m, dict(moss=20, grass=10, mush=10, bones=3)


def queens_lair():
    m = Map(38, 24, seed=66, fill="R", border="R")
    hubs = [(3, 3), (12, 6), (6, 15), (17, 19), (20, 11), (27, 4), (29, 15), (14, 2)]
    for a, b in [(0, 1), (1, 4), (0, 2), (2, 3), (3, 4), (4, 5), (1, 7), (7, 5), (4, 6)]:
        m.walk(hubs[a], hubs[b], ".", width=2, wobble=0.4)
    for x, y in hubs:
        m.blob(x, y, m.rng.uniform(1.8, 2.8), ".")
    m.rect(32, 8, 36, 19, ".")                     # the queen's chamber
    m.walk((29, 15), (32, 15), ".", width=1, wobble=0.0)
    m.set(31, 15, "D")
    m.rect(31, 13, 31, 14, "R")
    m.rect(31, 16, 31, 17, "R")
    m.set(3, 3, "A")
    m.set(6, 16, "K")
    m.set(34, 13, "g")
    m.set(34, 9, "g")
    m.set(17, 20, "g")
    m.set(27, 4, "g")
    m.scatter("N", 3, (8, 4, 30, 21), clear=3)
    m.set(35, 17, "N")
    m.scatter("G", 6, away_from=(3, 3), min_dist=12)
    m.scatter("G", 2, (32, 8, 36, 19))
    m.set(34, 11, "P")
    return m, dict(dirt=12, bones=14, skull=8, moss=4, mush=4)


def hunted_hedges():
    """Braided hedge maze full of loops; four hunters and a locked gem vault you must come back out of."""
    m = Map(35, 21, seed=77, border="H")
    m.maze(0, 0, 34, 20, "H", braid=1.0)
    for y in range(2, 19):  # knock out ~40% of the hedge segments: loops everywhere
        for x in range(2, 33):
            if (x + y) % 2 == 1 and m.get(x, y) == "H" and m.rng.random() < 0.4:
                m.set(x, y, ".")
    m.rect(14, 8, 20, 12, "W", filled=False)       # the vault
    m.rect(15, 9, 19, 11, ".")
    m.set(17, 8, "D")
    m.set(1, 1, "A")
    m.set(33, 19, "K")
    for p in [(16, 10), (18, 10), (33, 1), (1, 19), (25, 5), (9, 15)]:
        m.set(*p, "g")
    for p in [(26, 3), (31, 11), (7, 17), (23, 17), (21, 5), (13, 13)]:
        m.set(*p, "P")
    return m, dict(grass=4, dirt=8, mush=5)


def spider_queens_court():
    """Open courtyards joined by narrow gates; brood nests keep hatching hunters while you search."""
    m = Map(39, 23, seed=88, fill="R", border="R")
    for x0, y0, x1, y1 in [(1, 1, 11, 9), (14, 1, 24, 9), (27, 1, 37, 9), (1, 13, 11, 21), (14, 13, 24, 21),
                           (27, 13, 37, 21)]:
        m.rect(x0, y0, x1, y1, ".")
        cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
        m.rect(cx - 1, cy - 1, cx + 1, cy + 1, "t" if m.rng.random() < 0.5 else "~")   # a pillar to loop around
    for y in (3, 7, 15, 19):                       # gates between courtyards (two per wall: loops)
        for x in (12, 13, 25, 26):
            m.set(x, y, ".")
    for x in (4, 8, 17, 21, 32):
        for y in (10, 11, 12):
            m.set(x, y, ".")
    m.rect(27, 13, 37, 13, "W")                    # the queen's courtyard is walled off...
    m.rect(26, 13, 26, 21, "W")
    m.set(32, 13, "D")                             # ...behind one door
    for y in (15, 19):
        m.set(26, y, "W")
        m.set(25, y, "W")
    m.set(2, 2, "A")
    m.set(36, 2, "K")
    for p in [(36, 20), (28, 20), (20, 20), (2, 20), (20, 2), (36, 8)]:
        m.set(*p, "g")
    for p in [(19, 19), (35, 5), (6, 19), (18, 5), (5, 15)]:
        m.set(*p, "P")
    m.set(32, 19, "B")
    m.set(10, 15, "B")
    m.scatter("G", 3, away_from=(2, 2), min_dist=14)
    return m, dict(dirt=12, bones=14, skull=8, moss=4, mush=4)


LEVELS = [
    ("Garden Gate", "Find the key to open the walled garden.", garden_gate),
    ("Riverlands", "Cross the river by bridge — nests are hatching on the far bank.", riverlands),
    ("Hedge Maze", "Four gems hidden in a looping hedge maze - and a hunter that tracks you through it.", hedge_maze),
    ("Sunken Crypt", "Tombs, torches and a locked treasure room.", sunken_crypt),
    ("The Bog", "Wade through the mire. Watch the spiders in the reeds.", bog),
    ("Queen's Lair", "Deep caves, many nests. A hunter guards the queen's treasure.", queens_lair),
    ("Hunted Hedges", "☠ Nightmare: six red-eyed hunters track you through the hedges. Use the loops, hide in "
     "the grass, and get the key for the vault.", hunted_hedges),
    ("Spider Queen's Court", "☠ Nightmare: courtyards and narrow gates, hunters and brood nests that hatch more. "
     "The last gems wait behind the queen's door.", spider_queens_court),
]


# ----------------------------------------------------------------------------- validation
def reach(rows, start, opened=frozenset()):
    seen, q = {start}, collections.deque([start])
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            c = rows[ny][nx] if 0 <= ny < len(rows) and 0 <= nx < len(rows[0]) else "W"
            if (nx, ny) in seen:
                continue
            if c in BLOCKING and not (c == "D" and (nx, ny) in opened):
                continue
            seen.add((nx, ny))
            q.append((nx, ny))
    return seen


def validate(name, rows):
    w = len(rows[0])
    assert all(len(r) == w for r in rows), f"{name}: ragged rows"
    cells = {(x, y): c for y, r in enumerate(rows) for x, c in enumerate(r)}
    a = [p for p, c in cells.items() if c == "A"]
    assert len(a) == 1, f"{name}: need exactly one A"
    gems = [p for p, c in cells.items() if c == "g"]
    keys = [p for p, c in cells.items() if c == "K"]
    doors = [p for p, c in cells.items() if c == "D"]
    assert gems, f"{name}: no gems"
    assert len(keys) >= len(doors), f"{name}: not enough keys for doors"
    opened: set = set()
    seen = reach(rows, a[0])
    while True:  # open doors with keys we can reach, until nothing changes
        usable = sum(k in seen for k in keys) - len(opened)
        frontier = [d for d in doors if d not in opened and any((d[0] + dx, d[1] + dy) in seen
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))]
        if usable <= 0 or not frontier:
            break
        opened.add(frontier[0])
        seen = reach(rows, a[0], frozenset(opened))
    missing = [p for p in gems if p not in seen]
    assert not missing, f"{name}: unreachable gems {missing}"
    ax, ay = a[0]
    near = [p for p, c in cells.items() if c in "GNPB" and abs(p[0] - ax) + abs(p[1] - ay) <= 4]
    assert not near, f"{name}: spider/nest within 4 of start {near}"
    return (len(gems), len(keys), sum(c == "G" for c in cells.values()), sum(c == "N" for c in cells.values()),
            sum(c == "P" for c in cells.values()), sum(c == "B" for c in cells.values()))


# ----------------------------------------------------------------------------- decoration
def decorate(rows, recipe, seed):
    rng = random.Random(seed)
    h, w = len(rows), len(rows[0])
    ground = [[""] * w for _ in range(h)]
    walkable = [(x, y) for y in range(h) for x in range(w) if rows[y][x] not in STRUCTURE]

    def blob(char, count, rmin, rmax):
        for _ in range(count):
            cx, cy = rng.choice(walkable)
            r = rng.uniform(rmin, rmax)
            for x, y in walkable:
                d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
                if d <= r and rng.random() < 1.15 - d / (r + 0.01) * 0.6:
                    ground[y][x] = char

    blob("d", recipe.get("dirt", 0), 1.0, 2.8)
    blob("m", recipe.get("moss", 0), 1.0, 3.0)
    blob(",", recipe.get("grass", 0), 1.2, 3.4)
    for y in range(h):  # grass grows around bushes and trees
        for x in range(w):
            if rows[y][x] in "bT":
                for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if (nx, ny) in set(walkable) and rng.random() < (1 if (dx, dy) == (0, 0) else 0.5):
                        ground[ny][nx] = ","
    for char, key in (("u", "mush"), ("x", "skull"), ("o", "bones")):
        free = [(x, y) for x, y in walkable if rows[y][x] == "." and ground[y][x] in ("", "m", "d")]
        for x, y in rng.sample(free, min(recipe.get(key, 0), len(free))):
            ground[y][x] = char
    return ground


def level_tokens(rows, ground):
    out = []
    for y, r in enumerate(rows):
        toks = []
        for x, c in enumerate(r):
            gch = ground[y][x]
            if c in STRUCTURE:
                toks.append(c)
            elif c == ".":
                toks.append(gch or ".")
            else:  # entity or prop drawn over the ground
                toks.append(f"{gch}/{c}" if gch and gch not in "uxo" else c)
        out.append(" ".join(t.ljust(3) for t in toks).rstrip())
    return "\n".join(out)


# ----------------------------------------------------------------------------- GDY text
HEADER = '''Version: "0.1"
Environment:
  Name: Spiders Garden
  Description: |
    Collect every gem without touching a spider. Spiders wander at random and nests hatch more of
    them. Red-eyed hunters track you down (A* pathfinding) once you come within 9 tiles, one step every
    3 ticks (every 2 once you hold 3 gems); they lose you beyond 14 tiles or when you hide in tall grass. Brood nests hatch hunters.
    Keys open doors. Water, walls, hedges, rocks, bushes, trees, tombstones, torches and nests
    block the way; grass, moss, dirt and bridges are walkable.
    Generated by studio/griddly-demo/tools/gen_spiders_garden.py — edit that, not this file.
  # Cameras are observers. The demo picks one by name and the engine renders it; nothing about
  # framing lives in JS.
  #   Sprite2D    whole level (global observer)
  #   GnomeCam    15x11 window that follows the gnome
  #   GnomeCloseUp 9x9 window that follows the gnome (also the agent's default view)
  Observers:
    Sprite2D:
      TileSize: 24
      BackgroundTile: oryx/oryx_fantasy/floor3-0.png
      RotateAvatarImage: false
      TrackAvatar: false
    GnomeCam:
      Type: SPRITE_2D
      TileSize: 24
      BackgroundTile: oryx/oryx_fantasy/floor3-0.png
      RotateAvatarImage: false
      RotateWithAvatar: false
      TrackAvatar: true
      Width: 15
      Height: 11
    GnomeCloseUp:
      Type: SPRITE_2D
      TileSize: 24
      BackgroundTile: oryx/oryx_fantasy/floor3-0.png
      RotateAvatarImage: false
      RotateWithAvatar: false
      TrackAvatar: true
      Width: 9
      Height: 9
    Block2D:
      TileSize: 24
  Variables:
    - Name: ax
      InitialValue: 0
    - Name: ay
      InitialValue: 0
    - Name: hidden
      InitialValue: 0
    - Name: taken
      InitialValue: 0
  Player:
    Observer:
      TrackAvatar: true
      RotateWithAvatar: false
      RotateAvatarImage: false
      Height: 9
      Width: 9
    AvatarObject: gnome
  Termination:
    Win:
      - eq: [gem:count, 0]
    Lose:
      - eq: [gnome:count, 0]
  Levels:
'''

GROUND = "[grass, moss, dirt, bones, mushroom, skull, bridge]"
SOLID = "[wall, hedge, rock, water, door, bush, tree, tombstone, torch, nest, brood, gem, key, hunter]"
ALL_OBJECTS = ["wall", "hedge", "rock", "water", "bridge", "door", "grass", "moss", "dirt", "bones", "mushroom", "skull",
               "bush", "tree", "tombstone", "torch", "nest", "brood", "key", "gem", "spider", "hunter", "gnome"]
K.GROUND = {"grass", "moss", "dirt", "bones", "mushroom", "skull", "bridge"}
# the gnome walking onto tall grass sets `hidden` (hunters more than 2 tiles away lose track)
SYNC_MOVE = "{exec: {Action: sync, ActionId: 1}}"


FIX_GROUND = lambda t: t.replace("[_empty, grass, moss, dirt, sand, bones, mushroom, skull, bridge]",
                                 "[_empty, grass, moss, dirt, bones, mushroom, skull, bridge]")
HATCH_HUNTER = FIX_GROUND(K.hatch("hatch_hunter", "brood", "hunter", 90, 15, ALL_OBJECTS, limit=2))
HUNTER = FIX_GROUND(K.hunter("hunter", "gnome", ALL_OBJECTS, {
    "gnome": "      - Src: {Object: hunter}\n        Dst: {Object: gnome, Commands: [{remove: true}, {reward: -1}]}\n"},
    sense=9, lose=14, pace=3, hide=True, fast=("taken", 3, 2)))


def exec_(action, delay=3):
    return f"{{Action: {action}, Delay: {delay}, Randomize: true}}"


ACTIONS = f'''
Actions:
  # Spiders wander: rotate left/right or step forwards, at random.
  - Name: random_movement
    InputMapping:
      Inputs:
        1: {{Description: Rotate left, OrientationVector: [-1, 0]}}
        2: {{Description: Move forwards, OrientationVector: [0, -1], VectorToDest: [0, -1]}}
        3: {{Description: Rotate right, OrientationVector: [1, 0]}}
      Relative: true
      Internal: true
    Behaviours:
      - Src: {{Object: spider, Commands: [{{rot: _dir}}, {{exec: {exec_("random_movement")}}}]}}
        Dst: {{Object: spider}}
      - Src: {{Object: spider, Commands: [{{mov: _dest}}, {{exec: {exec_("random_movement")}}}]}}
        Dst: {{Object: _empty}}
      - Src: {{Object: spider, Commands: [{{mov: _dest}}, {{exec: {exec_("random_movement")}}}]}}
        Dst: {{Object: {GROUND}}}
      - Src: {{Object: spider, Commands: [{{exec: {exec_("random_movement")}}}]}}
        Dst: {{Object: {SOLID}}}
      - Src: {{Object: spider}}
        Dst: {{Object: gnome, Commands: [{{remove: true}}, {{reward: -1}}]}}

  # Nests hatch up to 2 spiders each, into a random neighbouring cell.
  - Name: hatch
    InputMapping:
      Internal: true
    Behaviours:
      - Src:
          Object: nest
          Preconditions: [{{lt: [src.hatched, 2]}}]
          Commands: [{{spawn: spider}}, {{incr: hatched}}, {{exec: {exec_("hatch", 60)}}}]
        Dst: {{Object: _empty}}
      - Src:
          Object: nest
          Preconditions: [{{lt: [src.hatched, 2]}}]
          Commands: [{{spawn: spider}}, {{incr: hatched}}, {{exec: {exec_("hatch", 60)}}}]
        Dst: {{Object: {GROUND}}}
      - Src:
          Object: nest
          Preconditions: [{{lt: [src.hatched, 2]}}]
          Commands: [{{exec: {exec_("hatch", 10)}}}]
        Dst: {{Object: [wall, hedge, rock, water, door, bush, tree, tombstone, torch, nest, brood, gem, key, spider, hunter, gnome]}}

  # Brood nests hatch up to 2 hunters each.
{HATCH_HUNTER}
  # The gnome publishes its position (ax, ay) and whether it stands in tall grass (hidden).
  - Name: sync
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Sync, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: gnome, Commands: [{{set: [ax, _x]}}, {{set: [ay, _y]}}, {{set: [hidden, 0]}}]}}
        Dst: {{Object: gnome}}
  - Name: hide
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Hide, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: gnome, Commands: [{{set: [hidden, 1]}}]}}
        Dst: {{Object: gnome}}

  # Hunters: one step every 3 ticks towards the gnome (A*) while alert, else a random step.
{HUNTER}

  # Start the gnome facing down (render tile 2).
  - Name: face
    InputMapping:
      Internal: true
      Inputs:
        2: {{Description: Face down, VectorToDest: [0, 0], MetaData: {{facing: 2}}}}
    Behaviours:
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: gnome}}

  # The gnome moves up / down / left / right (absolute; it never rotates). MetaData.facing picks the
  # gnome's render tile (set_tile) so every observer shows which way it last moved.
  - Name: move
    InputMapping:
      Inputs:
        1: {{Description: Left, VectorToDest: [-1, 0], MetaData: {{facing: 3}}}}
        2: {{Description: Up, VectorToDest: [0, -1], MetaData: {{facing: 0}}}}
        3: {{Description: Right, VectorToDest: [1, 0], MetaData: {{facing: 1}}}}
        4: {{Description: Down, VectorToDest: [0, 1], MetaData: {{facing: 2}}}}
    Behaviours:
      - Src: {{Object: gnome, Commands: [{{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: [spider, hunter]}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {SYNC_MOVE}]}}
        Dst: {{Object: _empty}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {SYNC_MOVE}, {{exec: {{Action: hide, ActionId: 1}}}}]}}
        Dst: {{Object: grass}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {SYNC_MOVE}]}}
        Dst: {{Object: [moss, dirt, bones, mushroom, skull, bridge]}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}, {{incr: taken}}]}}
        Dst: {{Object: gem, Commands: [{{remove: true}}]}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}, {{incr: keys}}]}}
        Dst: {{Object: key, Commands: [{{remove: true}}]}}
      - Src:
          Object: gnome
          Preconditions: [{{gte: [src.keys, 1]}}]
          Commands: [{{set_tile: meta.facing}}, {{decr: keys}}]
        Dst: {{Object: door, Commands: [{{remove: true}}]}}
      - Src: {{Object: gnome, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, hedge, rock, water, bush, tree, tombstone, torch, nest, brood]}}
'''


SPRITE_CAMERAS = ["GnomeCam", "GnomeCloseUp"]


def obj(name, char, z, sprite, color, shape="square", scale=1.0, extra="", tiles=1):
    shared = "".join(f"      {cam}: *{name}_sprite\n" for cam in SPRITE_CAMERAS)
    block = "\n".join(f"        - Shape: {shape}\n          Color: {color}\n          Scale: {scale}" for _ in range(tiles))
    return f'''  - Name: {name}
    MapCharacter: '{char}'
    Z: {z}
{extra}    Observers:
      Sprite2D: &{name}_sprite
{sprite}
{shared}      Block2D:
{block}
'''


def img(path):
    return f"        - Image: {path}"


def tiled(prefix, sep="-"):
    imgs = "\n".join(f"            - {prefix}{sep}{i}.png" for i in range(16))
    return f"        - TilingMode: WALL_16\n          Image:\n{imgs}"


TG = "oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_"
SPIDER_INIT = "    InitialActions:\n      - Action: random_movement\n        Delay: 3\n        Randomize: true\n"
NEST_INIT = ("    Variables:\n      - Name: hatched\n        InitialValue: 0\n"
             "    InitialActions:\n      - Action: hatch\n        Delay: 40\n        Randomize: true\n")
GNOME_VARS = ("    Variables:\n      - Name: keys\n        InitialValue: 0\n"
              "    InitialActions:\n      - Action: face\n        ActionId: 2\n      - Action: sync\n        ActionId: 1\n")
BROOD_INIT = NEST_INIT.replace("Action: hatch\n        Delay: 40", "Action: hatch_hunter\n        Delay: 45")

OBJECTS = "\nObjects:\n" + "".join([
    obj("wall", "W", 0, tiled("oryx/oryx_fantasy/wall2"), "[0.45, 0.45, 0.45]"),
    obj("hedge", "H", 0, tiled("oryx/oryx_fantasy/wall9"), "[0.15, 0.35, 0.15]"),
    obj("rock", "R", 0, tiled("gvgai/oryx/dirtWall", "_"), "[0.45, 0.25, 0.12]"),
    obj("water", "~", 0, img(TG + "water.png"), "[0.1, 0.35, 0.7]"),
    obj("bridge", "=", 0, img("gvgai/newset/logm.png"), "[0.55, 0.4, 0.25]"),
    obj("door", "D", 0, img("oryx/oryx_fantasy/door-1.png"), "[0.6, 0.4, 0.1]"),
    obj("grass", ",", 0, tiled("gvgai/oryx/grass", "_"), "[0.25, 0.55, 0.25]", scale=0.9),
    obj("moss", "m", 0, img(TG + "floor_moss_a.png"), "[0.25, 0.4, 0.2]", scale=0.9),
    obj("dirt", "d", 0, img(TG + "floor_dirt_a.png"), "[0.3, 0.22, 0.15]", scale=0.9),
    obj("bones", "o", 0, img(TG + "bones_a.png"), "[0.8, 0.8, 0.75]", "circle", 0.3),
    obj("mushroom", "u", 0, img("gvgai/oryx/mushroom2.png"), "[0.8, 0.2, 0.2]", "circle", 0.3),
    obj("skull", "x", 0, img("gvgai/oryx/skull1.png"), "[0.9, 0.9, 0.9]", "circle", 0.3),
    obj("bush", "b", 1, img("gvgai/oryx/bush1.png"), "[0.1, 0.45, 0.15]", "circle", 0.8),
    obj("tree", "T", 1, img("gvgai/oryx/tree2.png"), "[0.05, 0.4, 0.2]", "circle", 0.9),
    obj("tombstone", "t", 1, img("gvgai/oryx/tombstone1.png"), "[0.5, 0.5, 0.5]", "square", 0.7),
    obj("torch", "i", 1, img("gvgai/oryx/torch1.png"), "[1.0, 0.6, 0.1]", "circle", 0.5),
    obj("nest", "N", 1, img("gvgai/newset/cocoonb1.png"), "[0.6, 0.6, 0.5]", "circle", 0.7, NEST_INIT),
    obj("brood", "B", 1, img("gvgai/newset/cocoonb1.png"), "[0.6, 0.0, 0.0]", "circle", 0.7, BROOD_INIT),
    obj("key", "K", 1, img("oryx/oryx_fantasy/key-0.png"), "[1.0, 0.85, 0.1]", "triangle", 0.5),
    obj("gem", "g", 1, img("oryx/oryx_fantasy/ore-6.png"), "[0.0, 1.0, 0.0]", "triangle", 0.5),
    obj("spider", "G", 2, img("oryx/oryx_fantasy/avatars/spider1.png"), "[1.0, 0.0, 0.0]", "triangle", 0.8, SPIDER_INIT),
    obj("hunter", "P", 2, img("gvgai/oryx/spider2.png") + "\n" + img("gvgai/oryx/spider1.png"), "[1.0, 0.5, 0.0]",
        "triangle", 0.8, K.hunter_object_extra(), tiles=2),
    obj("gnome", "A", 2, "\n".join(img(f"spiders-garden/gnome-{d}.png") for d in ("up", "right", "down", "left")),
        "[0.0, 0.0, 1.0]", "triangle", 0.8, GNOME_VARS, tiles=4),
])


def main():
    parts = [HEADER]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, recipe = build()
        rows = m.rows()  # (crops m.w/m.h)
        gems, keys, spiders, nests, hunters, broods = validate(name, rows)
        body = level_tokens(rows, decorate(rows, recipe, seed=1000 + i))
        parts.append(f"    # {name} | {blurb}\n    - |\n" + "\n".join("      " + r for r in body.split("\n")) + "\n")
        print(f"{i + 1}. {name:13s} {m.w}x{m.h}  gems={gems} keys={keys} spiders={spiders} nests={nests} hunters={hunters} broods={broods}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + OBJECTS
    for p in OUT:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(gdy)
        print(f"wrote {p.relative_to(ROOT)}", file=sys.stderr)


if __name__ == "__main__":
    main()
