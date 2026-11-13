"""Map generator for GriddlyRTS-FoW ("Frontiers").

Every map is a two-player map that is point-symmetric (rotated 180 degrees about the centre), so
neither side has a terrain advantage. Each map has its own style - the terrain mix, layout of
chokepoints and where the valuable resources are - but they all share the same ingredients:

    A / H     base and two starting workers per player (player id appended, e.g. A1, H2)
    M         mineral field (15 ore per patch)
    G         gem deposit (worth 3 resources per trip, few of them, usually contested)
    R         neutral relay: stand next to it to capture it; it pays its owner income
    W         rock / wall          (blocks everything)
    ~         water                (blocks everything; gunners can shoot over it)
    T         forest               (blocks units; workers can cut it down for wood -> 1 resource)
    =         bridge               (walkable, cannot be built on)
    .         open ground

`python fow_maps.py` prints every map with its stats.
"""
import random
from collections import deque

OPEN = "."
SOLID = set("W~T")


class Canvas:
    """One half of a symmetric map is painted; `mirror()` completes it."""

    def __init__(self, w, h, seed):
        self.w, self.h = w, h
        self.rng = random.Random(seed)
        self.g = [[OPEN] * w for _ in range(h)]

    def inside(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def set(self, x, y, c):
        if self.inside(x, y):
            self.g[y][x] = c

    def get(self, x, y):
        return self.g[y][x]

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def blob(self, cx, cy, r, c, jitter=0.3, only=None):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if not self.inside(x, y):
                    continue
                d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
                if d <= r * (1 - jitter / 2 + self.rng.random() * jitter) and (only is None or self.g[y][x] in only):
                    self.g[y][x] = c

    def line(self, p0, p1, c, width=1, wobble=0.0, only=None):
        """Wobbly brush stroke from p0 to p1 (rivers, ridges, roads)."""
        x, y = p0
        tx, ty = p1
        for _ in range(6 * (self.w + self.h)):
            for dy in range(-(width // 2), width - width // 2):
                for dx in range(-(width // 2), width - width // 2):
                    if self.inside(x + dx, y + dy) and (only is None or self.g[y + dy][x + dx] in only):
                        self.g[y + dy][x + dx] = c
            if (x, y) == (tx, ty):
                return
            if wobble and self.rng.random() < wobble:
                x += self.rng.choice((-1, 0, 1))
                y += self.rng.choice((-1, 0, 1))
            elif abs(tx - x) >= abs(ty - y):
                x += (tx > x) - (tx < x)
            else:
                y += (ty > y) - (ty < y)
            x = min(max(x, 0), self.w - 1)
            y = min(max(y, 0), self.h - 1)

    def scatter(self, c, n, region, only=OPEN, min_gap=0):
        x0, y0, x1, y1 = region
        placed = 0
        for _ in range(n * 40):
            if placed >= n:
                break
            x, y = self.rng.randint(x0, x1), self.rng.randint(y0, y1)
            if not self.inside(x, y) or self.g[y][x] not in only:
                continue
            if min_gap and any(self.inside(x + dx, y + dy) and self.g[y + dy][x + dx] == c
                               for dx in range(-min_gap, min_gap + 1) for dy in range(-min_gap, min_gap + 1)):
                continue
            self.g[y][x] = c
            placed += 1

    def mirror(self):
        """Point symmetry: the lower-right half becomes a 180-degree copy of the upper-left half."""
        w, h = self.w, self.h
        for y in range(h):
            for x in range(w):
                # cells "before" the centre in reading order are the source
                if y * w + x < (h - 1 - y) * w + (w - 1 - x):
                    c = self.g[y][x]
                    self.g[h - 1 - y][w - 1 - x] = c.replace("1", "2") if c[-1:] == "1" else c

    def clear(self, x, y, r, keep=()):
        for yy in range(y - r, y + r + 1):
            for xx in range(x - r, x + r + 1):
                if self.inside(xx, yy) and self.g[yy][xx] not in keep:
                    self.g[yy][xx] = OPEN

    def start(self, bx, by, minerals, gems=()):
        """Player 1's base, two workers and its starting resources (mirrored for player 2)."""
        self.clear(bx, by, 3)
        for x, y in list(minerals) + list(gems):
            self.clear(x, y, 1)
        self.g[by][bx] = "A1"
        self.g[by][bx + 1] = "H1"
        self.g[by + 1][bx] = "H1"
        for x, y in minerals:
            self.set(x, y, "M")
        for x, y in gems:
            self.set(x, y, "G")

    def rows(self):
        return self.g


def mineral_patch(cx, cy, shape="L"):
    """A small field of 5-6 mineral patches around (cx, cy)."""
    shapes = {
        "L": [(0, 0), (1, 0), (2, 0), (0, 1), (1, 1), (0, 2)],
        "row": [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0)],
        "col": [(0, 0), (0, 1), (0, 2), (0, 3), (0, 4)],
        "ring": [(0, 0), (1, 0), (2, 0), (0, 1), (2, 1)],
        "clump": [(0, 0), (1, 0), (0, 1), (1, 1)],
    }
    return [(cx + dx, cy + dy) for dx, dy in shapes[shape]]


# ----------------------------------------------------------------------------------------------
# Map styles
# ----------------------------------------------------------------------------------------------

def crossroads():
    """Open plains, two roads cross the centre; relays on the roads, gems in the middle."""
    c = Canvas(48, 32, 11)
    for _ in range(14):
        c.blob(c.rng.randint(4, 44), c.rng.randint(3, 28), c.rng.uniform(1.0, 2.2), "T")
    for _ in range(6):
        c.blob(c.rng.randint(8, 40), c.rng.randint(4, 27), c.rng.uniform(0.8, 1.6), "W")
    c.line((0, 15), (47, 16), OPEN, width=3)
    c.line((23, 0), (24, 31), OPEN, width=3)
    c.start(4, 4, mineral_patch(1, 1), gems=[])
    c.set(10, 2, "M"), c.set(11, 2, "M"), c.set(10, 3, "M")
    c.clear(23, 15, 2)
    c.set(22, 14, "G"), c.set(23, 13, "G")
    c.set(12, 15, "R"), c.set(23, 6, "R")
    c.scatter("M", 4, (14, 4, 20, 10), min_gap=0)
    c.mirror()
    return "Crossroads", "Open plains and two roads. Fast attacks; relays line the roads, gems sit at the crossing.", c


def river_delta():
    """A river splits the map; three bridges are the only crossings."""
    c = Canvas(56, 40, 23)
    c.line((0, 33), (55, 6), "~", width=4, wobble=0.25)
    for bx, by in ((14, 27), (27, 20), (40, 13)):
        c.rect(bx - 3, by - 3, bx + 3, by + 3, "=")
    # bridges only span the river: bridge tiles left on dry land go back to ground
    for y in range(c.h):
        for x in range(c.w):
            if c.g[y][x] == "=":
                wet = any(c.inside(x + dx, y + dy) and c.g[y + dy][x + dx] == "~" for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                if not wet:
                    c.g[y][x] = OPEN
    for _ in range(18):
        c.blob(c.rng.randint(2, 54), c.rng.randint(2, 38), c.rng.uniform(1.0, 2.0), "T", only={OPEN})
    c.start(4, 4, mineral_patch(1, 1))
    c.scatter("M", 5, (12, 2, 20, 8))
    c.set(26, 14, "G"), c.set(27, 14, "G")
    c.set(6, 20, "R"), c.set(30, 4, "R")
    c.mirror()
    return "River Delta", "A wide river with three bridges. Hold the crossings; gunners can shoot across the water.", c


def highlands():
    """Rock ridges with narrow passes; rich gem plateau in the middle."""
    c = Canvas(48, 48, 37)
    for i in range(5):
        y = 8 + i * 8
        c.line((0, y), (47, y + c.rng.randint(-3, 3)), "W", width=2, wobble=0.15)
    for i in range(5):
        y = 8 + i * 8
        for _ in range(2):
            gx = c.rng.randint(4, 43)
            c.rect(gx - 1, y - 3, gx + 1, y + 4, OPEN)
    c.start(4, 3, mineral_patch(1, 1, "row") + [(1, 2), (2, 2)])
    c.clear(23, 23, 3)
    c.rect(21, 21, 26, 26, OPEN)
    for x, y in ((22, 22), (23, 22), (22, 23), (25, 25)):
        c.set(x, y, "G")
    c.set(10, 13, "R"), c.set(37, 13, "R"), c.set(23, 4, "R")
    c.scatter("M", 6, (2, 12, 20, 20))
    c.scatter("T", 20, (0, 0, 47, 23))
    c.mirror()
    return "Highlands", "Rock ridges cut the map into bands joined by narrow passes. A gem plateau sits in the centre.", c


def archipelago():
    """Islands linked by bridges across open water."""
    c = Canvas(60, 40, 51)
    c.rect(0, 0, 59, 39, "~")
    islands = [(7, 7, 6), (24, 8, 4.5), (13, 22, 4.5), (30, 20, 5.5), (45, 9, 4)]
    for x, y, r in islands:
        c.blob(x, y, r, OPEN, jitter=0.15)
    links = [((7, 7), (24, 8)), ((7, 7), (13, 22)), ((24, 8), (30, 20)), ((13, 22), (30, 20)), ((24, 8), (45, 9))]
    for a, b in links:
        c.line(a, b, "=", width=2, only={"~"})
    c.start(6, 6, mineral_patch(3, 2, "row"))
    c.set(12, 7, "M"), c.set(12, 8, "M")
    c.scatter("M", 5, (21, 6, 27, 11))
    c.scatter("M", 4, (10, 19, 16, 25))
    c.set(29, 19, "G"), c.set(31, 19, "G"), c.set(30, 22, "G")
    c.set(45, 9, "R"), c.set(13, 22, "R")
    c.scatter("T", 10, (0, 0, 59, 19))
    c.mirror()
    return "Archipelago", "Islands joined by narrow bridges. Expand island by island; the central island holds gems.", c


def deep_forest():
    """Dense forest with cleared paths; workers can chop new routes."""
    c = Canvas(52, 36, 64)
    c.rect(0, 0, 51, 35, "T")
    for _ in range(9):
        c.blob(c.rng.randint(6, 46), c.rng.randint(4, 31), c.rng.uniform(2.5, 4.0), OPEN)
    paths = [((4, 4), (25, 17)), ((4, 4), (20, 4)), ((4, 4), (6, 28)), ((20, 4), (25, 17)), ((6, 28), (25, 17)),
             ((20, 4), (45, 6))]
    for a, b in paths:
        c.line(a, b, OPEN, width=2, wobble=0.3)
    c.start(4, 4, mineral_patch(1, 1))
    c.clear(25, 17, 3)
    c.set(25, 17, "G"), c.set(26, 17, "G")
    c.set(20, 4, "R"), c.set(6, 28, "R")
    c.scatter("M", 5, (16, 2, 24, 8))
    c.mirror()
    return "Deep Forest", "Thick forest with a few winding trails. Workers can cut wood to open shortcuts.", c


def canyon():
    """Long narrow canyon: a single main corridor with side pockets of gems."""
    c = Canvas(64, 24, 77)
    c.rect(0, 0, 63, 23, "W")
    c.line((3, 12), (60, 11), OPEN, width=6, wobble=0.35)
    for x in (14, 30, 46):
        c.rect(x - 2, 3, x + 2, 9, OPEN)
        c.line((x, 9), (x, 12), OPEN, width=2)
    c.start(4, 10, [(1, 9), (1, 10), (1, 11), (2, 13), (3, 14)])
    c.set(14, 4, "G"), c.set(13, 4, "G")
    c.set(30, 4, "G")
    c.scatter("M", 6, (10, 9, 22, 15))
    c.set(22, 12, "R"), c.set(46, 5, "R")
    c.mirror()
    return "Desert Canyon", "One long canyon with side pockets. Gems sit in the pockets; the main road is the only way through.", c


def fortress():
    """Each base sits in a walled fortress with two gates; relays outside."""
    c = Canvas(50, 34, 91)
    for _ in range(12):
        c.blob(c.rng.randint(4, 46), c.rng.randint(3, 30), c.rng.uniform(0.8, 1.8), "T")
    c.rect(0, 0, 14, 12, OPEN)
    for x in range(0, 15):
        c.set(x, 12, "W")
    for y in range(0, 13):
        c.set(14, y, "W")
    c.set(14, 5, OPEN), c.set(14, 6, OPEN), c.set(6, 12, OPEN), c.set(7, 12, OPEN)
    c.start(5, 4, mineral_patch(1, 1) + [(11, 1), (12, 1), (12, 2)])
    c.scatter("M", 6, (18, 2, 26, 8))
    c.set(24, 16, "G"), c.set(25, 16, "G")
    c.set(20, 14, "R"), c.set(4, 20, "R"), c.set(30, 4, "R")
    c.mirror()
    return "Twin Fortresses", "Both bases sit behind walls with two narrow gates. Towers at the gates hold off any rush.", c


MAPS = [crossroads, river_delta, highlands, archipelago, deep_forest, canyon, fortress]


def reachable(grid, start):
    h, w = len(grid), len(grid[0])
    seen = {start}
    q = deque([start])
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen and grid[ny][nx][0] not in "W~TMGR":
                seen.add((nx, ny))
                q.append((nx, ny))
    return seen


def validate(name, grid):
    """Both bases exist and every base, mineral, gem and relay is reachable from player 1's base."""
    cells = {(x, y): c for y, row in enumerate(grid) for x, c in enumerate(row)}
    b1 = [p for p, c in cells.items() if c == "A1"]
    b2 = [p for p, c in cells.items() if c == "A2"]
    assert len(b1) == 1 and len(b2) == 1, f"{name}: needs exactly one base per player"
    seen = reachable(grid, b1[0])
    def near(p):
        return any((p[0] + dx, p[1] + dy) in seen for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    assert near(b2[0]), f"{name}: player 2's base is unreachable"
    for p, c in cells.items():
        if c in ("M", "G", "R"):
            assert near(p), f"{name}: {c} at {p} is unreachable"


def build_all():
    out = []
    for fn in MAPS:
        name, blurb, canvas = fn()
        grid = canvas.rows()
        validate(name, grid)
        # map-size marker (sets the camera bounds) in the bottom-right corner; it lies under any
        # object there, so the corner is written as "?/<object>" when occupied
        corner = grid[-1][-1]
        grid[-1][-1] = "?" if corner == OPEN else f"?/{corner}"
        out.append((name, blurb, grid))
    return out


def level_text(grid, indent="      "):
    return "\n".join(indent + " ".join(c.ljust(2) for c in row).rstrip() for row in grid)


if __name__ == "__main__":
    for name, blurb, grid in build_all():
        counts = {}
        for row in grid:
            for c in row:
                counts[c[0]] = counts.get(c[0], 0) + 1
        print(f"== {name} {len(grid[0])}x{len(grid)}  {blurb}")
        print("   ", {k: v for k, v in sorted(counts.items()) if k != "."})
        for row in grid:
            print("   " + "".join(c[0] if c != OPEN else "." for c in row))
