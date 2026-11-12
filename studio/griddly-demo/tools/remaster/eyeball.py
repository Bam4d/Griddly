#!/usr/bin/env python3
"""Eyeball Remastered: a giant eye searches multi-room houses and dungeons for its eye drops.

Remaster of resources/games/Single-Player/Mini-Grid/minigrid-eyeball.yaml (MiniGrid 4-rooms). Each
level is a grid of 4-9 rooms joined by doorways (a random spanning tree plus a few extra loops),
themed with walls of brick / stone / hedge / rock and furnished with scenery. Bumping a bottle of
eye drops picks it up; the level is won when no eye drops are left (1-3 per level).

Layout characters (besides the shared terrain chars from common):
  W wall   A eyeball   g eye drops   . floor
Every level is validated: rectangular, one eyeball, every bottle reachable, and decoration never
disconnects floor (props are only kept if every previously reachable cell stays reachable).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

GID = "eyeball"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE = G["bridge"]
WALKABLE = set(".Ag") | set(G.values())
WALLS = "W" + HEDGE + STONE + ROCK


# ----------------------------------------------------------------------------- room helpers
def open_cells(m, start):
    return C.reach(["".join(r) for r in m.g], start, lambda c, p: c in WALKABLE)


def safe_paint(m, start, cells, ch):
    """Paint `cells` with `ch` only if no other reachable floor gets cut off. Returns success."""
    before = open_cells(m, start)
    cells = [p for p in cells if m.inside(*p) and m.get(*p) == "." and p != start]
    old = {p: m.get(*p) for p in cells}
    for p in cells:
        m.set(*p, ch)
    after = open_cells(m, start)
    if len(after) != len(before - set(cells)):
        for p, c in old.items():
            m.set(*p, c)
        return False
    return True


def props(m, start, ch, n, region=None, wall_side=False):
    """Scatter n solid props without disconnecting anything; wall_side = only next to a wall."""
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    for x, y in spots:
        if placed >= n:
            break
        if wall_side and not any(m.get(x + dx, y + dy) in WALLS for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            continue
        if safe_paint(m, start, [(x, y)], ch):
            placed += 1


def pool(m, start, cx, cy, r, ch=WATER):
    cells = [(x, y) for y in range(int(cy - r - 1), int(cy + r + 2)) for x in range(int(cx - r - 1), int(cx + r + 2))
             if ((x - cx) ** 2 + ((y - cy) * 1.15) ** 2) ** 0.5 <= r * (0.85 + 0.3 * m.rng.random())]
    return safe_paint(m, start, cells, ch)


def rooms(m, xs, ys, wall="W", extra=0.3, door=1):
    """Partition the box spanned by xs/ys (wall-line coordinates) into rooms, join them with doorways
    along a random spanning tree plus `extra` chance of loop doors. Returns [(x0, y0, x1, y1)] interiors
    indexed [row][col]."""
    for x in xs:
        m.rect(x, ys[0], x, ys[-1], wall)
    for y in ys:
        m.rect(xs[0], y, xs[-1], y, wall)
    nc, nr = len(xs) - 1, len(ys) - 1
    box = [[(xs[i] + 1, ys[j] + 1, xs[i + 1] - 1, ys[j + 1] - 1) for i in range(nc)] for j in range(nr)]

    def doorway(a, b):
        (i, j), (k, l) = a, b
        if j == l:  # horizontal neighbours: door in vertical wall x = xs[max(i,k)]
            x = xs[max(i, k)]
            y0, y1 = ys[j] + 1, ys[j + 1] - 1
            y = m.rng.randint(y0 + 1, max(y0 + 1, y1 - door))
            for d in range(door):
                m.g[min(y + d, y1)][x] = "."
        else:
            y = ys[max(j, l)]
            x0, x1 = xs[i] + 1, xs[i + 1] - 1
            x = m.rng.randint(x0 + 1, max(x0 + 1, x1 - door))
            for d in range(door):
                m.g[y][min(x + d, x1)] = "."

    cells = [(i, j) for j in range(nr) for i in range(nc)]
    start = m.rng.choice(cells)
    seen, stack, edges = {start}, [start], set()
    while stack:
        i, j = stack[-1]
        nb = [(i + di, j + dj) for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1))
              if 0 <= i + di < nc and 0 <= j + dj < nr and (i + di, j + dj) not in seen]
        if not nb:
            stack.pop()
            continue
        n = m.rng.choice(nb)
        doorway((i, j), n)
        edges.add(frozenset(((i, j), n)))
        seen.add(n)
        stack.append(n)
    for i, j in cells:
        for n in ((i + 1, j), (i, j + 1)):
            if n[0] < nc and n[1] < nr and frozenset(((i, j), n)) not in edges and m.rng.random() < extra:
                doorway((i, j), n)
    return box


def centre(b):
    return (b[0] + b[2]) // 2, (b[1] + b[3]) // 2


# ----------------------------------------------------------------------------- levels
def cottage():
    m = C.Map(25, 15, seed=11, border="W")
    box = rooms(m, [0, 11, 24], [0, 7, 14], extra=1.0)
    a = (3, 3)
    m.set(*a, "A")
    props(m, a, TORCH, 6, wall_side=True)
    props(m, a, BUSH, 3, box[1][0])
    pool(m, a, 17, 4, 1.3)
    m.set(20, 11, "g")
    return m, a, dict(dirt=3, grass=2, mush=2)


def manor():
    m = C.Map(33, 19, seed=22, border="W")
    box = rooms(m, [0, 9, 21, 32], [0, 8, 18], extra=0.4)
    a = centre(box[0][0])
    m.set(*a, "A")
    props(m, a, TORCH, 10, wall_side=True)
    props(m, a, TOMB, 3, box[1][2])
    pool(m, a, *centre(box[0][1]), 1.8)
    props(m, a, BUSH, 5)
    m.set(29, 16, "g")
    return m, a, dict(dirt=4, moss=3, sand=2, mush=3)


def greenhouse():
    m = C.Map(35, 21, seed=33, border=HEDGE)
    box = rooms(m, [0, 12, 23, 34], [0, 10, 20], wall=HEDGE, extra=0.5, door=2)
    a = (2, 2)
    m.set(*a, "A")
    for row in box:
        for b in row:
            if m.rng.random() < 0.6:
                pool(m, a, *centre(b), m.rng.uniform(1.2, 2.2))
            props(m, a, TREE, 3, b)
            props(m, a, BUSH, 4, b)
    m.set(*centre(box[1][2]), "g") if m.get(*centre(box[1][2])) == "." else m.set(31, 18, "g")
    m.set(17, 2, "g")
    return m, a, dict(grass=18, moss=4, mush=6)


def crypt():
    m = C.Map(37, 23, seed=44, fill=".", border=STONE)
    box = rooms(m, [0, 12, 24, 36], [0, 7, 15, 22], wall=STONE, extra=0.25)
    a = centre(box[0][0])
    m.set(*a, "A")
    for row in box:
        for b in row:
            for x in range(b[0] + 1, b[2], 2):  # rows of graves
                if m.rng.random() < 0.6:
                    props(m, a, TOMB, 1, (x, b[1] + 1, x, b[1] + 1))
    props(m, a, TORCH, 14, wall_side=True)
    for p in [(34, 20), (*centre(box[1][2]),)]:
        if m.get(*p) != ".":
            p = m.free(box[2][2])[0]
        m.set(*p, "g")
    return m, a, dict(dirt=10, moss=4, bones=14, skull=8)


def flooded_cellars():
    m = C.Map(41, 19, seed=55, border=ROCK)
    box = rooms(m, [0, 10, 20, 30, 40], [0, 9, 18], wall=ROCK, extra=0.3)
    a = (2, 2)
    m.set(*a, "A")
    for row in box:
        for b in row:
            pool(m, a, *centre(b), m.rng.uniform(1.8, 3.0))
            props(m, a, ROCK, 2, b)
    # a river channel along the middle row of a room bank, crossed by bridges
    for x in range(21, 40):
        if m.get(x, 4) == ".":
            m.set(x, 4, WATER)
    for x in (24, 35):
        m.set(x, 4, BRIDGE)
    props(m, a, TORCH, 6, wall_side=True)
    ok = open_cells(m, a)
    for p in [(38, 16), (27, 2), (14, 16)]:
        p = min((q for q in m.free() if q in ok), key=lambda q: abs(q[0] - p[0]) + abs(q[1] - p[1]))
        m.set(*p, "g")
    return m, a, dict(moss=14, dirt=6, mush=8, bones=4)


def eye_palace():
    m = C.Map(39, 25, seed=66, border="W")
    box = rooms(m, [0, 9, 23, 30, 38], [0, 6, 17, 24], extra=0.35)
    a = centre(box[1][1])                         # start in the great hall
    m.set(*a, "A")
    hall = box[1][1]
    for x in range(hall[0] + 2, hall[2] - 1, 3):  # colonnade of stone pillars
        for y in (hall[1] + 2, hall[3] - 2):
            safe_paint(m, a, [(x, y)], STONE)
    props(m, a, TORCH, 16, wall_side=True)
    props(m, a, TREE, 4, box[0][3])
    props(m, a, TOMB, 4, box[2][0])
    pool(m, a, *centre(box[2][2]), 1.5)
    pool(m, a, *centre(box[0][0]), 1.4)
    for b in (box[0][0], box[2][3], box[0][3]):
        spot = max(m.free(b), key=lambda q: abs(q[0] - a[0]) + abs(q[1] - a[1]))
        m.set(*spot, "g")
    return m, a, dict(dirt=6, sand=4, moss=4, mush=4, skull=3)


LEVELS = [
    ("Cottage", "Four little rooms. The drops are somewhere in here...", cottage),
    ("Manor", "Six rooms of an old manor house.", manor),
    ("Greenhouse", "Hedge-walled garden rooms full of ponds and plants. Two bottles.", greenhouse),
    ("Crypt", "Nine chambers of graves and torches. Two bottles.", crypt),
    ("Flooded Cellars", "Eight cellars, half under water. Three bottles.", flooded_cellars),
    ("Eye Palace", "A great hall and its wings. Three bottles to find.", eye_palace),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one eyeball"
    drops = C.find(rows, "g")
    assert drops, f"{name}: no eye drops"
    seen = C.reach(rows, a[0], lambda c, p: c in WALKABLE)
    missing = [p for p in drops if p not in seen]
    assert not missing, f"{name}: unreachable eye drops {missing}"
    return len(drops)


# ----------------------------------------------------------------------------- GDY
ACTIONS = f'''
Actions:
{C.face_action("eyeball")}
  # The eyeball rolls up / down / left / right; bumping the eye drops picks them up.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: eyeball, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: eyeball, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}]}}
        Dst: {{Object: eye_drops, Commands: [{{remove: true}}]}}
      - Src: {{Object: eyeball, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, water, tree, bush, rock, hedge, stone, tombstone, torch]}}
'''


def objects():
    avatar = C.facing_sprites("oryx/oryx_fantasy/avatars/eye1.png", "eyeball-eyeball")
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "W", 1, C.tiled("oryx/oryx_fantasy/wall7"), "[0.7, 0.7, 0.7]", "square", 1.0),
        C.obj("eye_drops", "g", 1, C.img("oryx/oryx_fantasy/potion-1.png"), "[0.0, 1.0, 0.0]", "square", 0.6),
        C.obj("eyeball", "A", 2, C.imgs(avatar), "[1.0, 0.0, 0.0]", "triangle", 0.9, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Eyeball Remastered", """
        Everything is yellow and it hurts to look at. Roll the giant eyeball through cottages, manors,
        greenhouses, crypts and flooded cellars to find every bottle of eye drops.
        Walls, water, trees, bushes, tombstones and torches block the way.
        """.replace("        ", ""), "eyeball", ["eq: [eye_drops:count, 0]"], [],
        background="oryx/oryx_fantasy/floor7-1.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid=WALLS)
        n = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=8000 + i, stack_under="Ag")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:16s} {len(rows[0])}x{len(rows)} drops={n}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    C.emit(GID, gdy, {
        "name": "Eyeball Remastered",
        "tagline": "Search room after room for your eye drops.",
        "description": "A giant eyeball searches multi-room houses, hedge greenhouses, crypts and flooded "
                       "cellars for its eye drops. Doorways link 4 to 9 rooms; find every bottle to win.",
        "tags": ["exploration", "maze", "classic-port"],
        "difficulty": "easy",
        "original": "Mini-Grid/minigrid-eyeball.yaml",
        "accent": "#facc15",
        "icon": "👁️",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Eye drops left", "kind": "remaining", "object": "eye_drops", "tone": "sky"}],
        "win_text": "Ahh, relief! Every bottle found.",
        "lose_text": "Out of time.",
    })


if __name__ == "__main__":
    main()
