#!/usr/bin/env python3
"""Spider Nest Remastered: a gnome braves volcanic caves full of lava and spider nests to collect
every gem.

Remaster of resources/games/Single-Player/GVGAI/spider-nest.yaml. Same core rules: spiders wander
(rotating at random) and kill the gnome on contact, nests hatch new spiders, lava kills the gnome and
any spider that blunders into it, and bumping a gem collects it (+1); the level is won when no gem is
left. Change: each nest hatches a limited brood (3 spiders) instead of forever, so the bigger caves
don't fill up completely.

Layout characters (besides the shared terrain chars from common):
  W cave wall   A gnome   g gem   N spider nest   S spider   L lava   . floor
Every level is validated: rectangular, one gnome, every gem reachable without crossing lava, and no
spider or nest within 6 tiles of the start.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import action_kit as K  # noqa: E402
import common as C  # noqa: E402

GID = "spider_nest"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE, DIRT, SAND = G["bridge"], G["dirt"], G["sand"]
WALK = set(".") | set(G.values())
SOLIDS = "W" + ROCK + STONE


def ok(c, p):
    return c in WALK or c in "AgS"


def safe_paint(m, a, cells, ch):
    """Paint cells unless that disconnects anything reachable (keeps the cave connected)."""
    cells = [p for p in cells if m.inside(*p) and m.get(*p) == "." and p != a]
    before = C.reach(K.grid_rows(m), a, ok)
    old = {p: m.get(*p) for p in cells}
    for p in cells:
        m.set(*p, ch)
    if len(C.reach(K.grid_rows(m), a, ok)) != len(before - set(cells)):
        for p, c in old.items():
            m.set(*p, c)
        return False
    return True


def lava_pool(m, a, cx, cy, r):
    cells = [(x, y) for y in range(int(cy - r - 1), int(cy + r + 2)) for x in range(int(cx - r - 1), int(cx + r + 2))
             if ((x - cx) ** 2 + ((y - cy) * 1.15) ** 2) ** 0.5 <= r * (0.85 + 0.3 * m.rng.random())]
    return safe_paint(m, a, cells, "L")


def lava_river(m, p0, p1, bridges):
    """A lava river from p0 to p1 (width 1), then log bridges at the given y (or x) positions."""
    m.walk(p0, p1, "L", width=1, wobble=0.3, only=".")
    vertical = abs(p1[1] - p0[1]) > abs(p1[0] - p0[0])
    for b in bridges:
        line = [(x, b) for x in range(1, m.w - 1)] if vertical else [(b, y) for y in range(1, m.h - 1)]
        for p in line:
            if m.get(*p) == "L":
                m.set(*p, BRIDGE)


def heal(m, a):
    """Reconnect any floor cut off by lava with bridges."""
    while True:
        rows = K.grid_rows(m)
        r = C.reach(rows, a, ok)
        cut = [p for p in m.free() if p not in r]
        if not cut:
            return
        tgt = min(cut, key=lambda p: min(abs(p[0] - q[0]) + abs(p[1] - q[1]) for q in r))
        x, y = min(r, key=lambda q: abs(tgt[0] - q[0]) + abs(tgt[1] - q[1]))
        while (x, y) != tgt:
            if x != tgt[0]:
                x += (tgt[0] > x) - (tgt[0] < x)
            else:
                y += (tgt[1] > y) - (tgt[1] < y)
            if m.get(x, y) == "L":
                m.set(x, y, BRIDGE)
            elif m.get(x, y) in SOLIDS:
                m.set(x, y, ".")


def populate(m, a, gems, nests, spiders, region=None, gem_spots=()):
    r = C.reach(K.grid_rows(m), a, ok)
    for p in gem_spots:
        m.set(*p, "g")
    for p in K.far_cells(m, a, gems, r, 6, region):
        m.set(*p, "g")
    spots = [p for p in m.free(region) if p in r and abs(p[0] - a[0]) + abs(p[1] - a[1]) >= 10
             and sum(m.get(p[0] + dx, p[1] + dy) == "." for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))) >= 3]
    m.rng.shuffle(spots)
    placed = 0
    for p in spots:
        if placed >= nests:
            break
        if safe_paint(m, a, [p], "N"):
            placed += 1
    for p in K.far_cells(m, a, spiders, C.reach(K.grid_rows(m), a, ok), 9, region):
        m.set(*p, "S")


def cave(m, hubs, links, width=2):
    for i, j in links:
        m.walk(hubs[i], hubs[j], ".", width=width, wobble=0.35)
    for x, y in hubs:
        m.blob(x, y, m.rng.uniform(2.0, 3.0), ".")


# ----------------------------------------------------------------------------- levels
def ember_hollow():
    m = C.Map(27, 16, seed=41, fill="W", border="W")
    cave(m, [(4, 4), (12, 8), (21, 4), (20, 12), (6, 12)], [(0, 1), (1, 2), (1, 3), (1, 4), (2, 3)])
    a = (4, 4)
    m.set(*a, "A")
    lava_pool(m, a, 12, 8, 1.4)
    heal(m, a)
    safe_paint(m, a, [(5, 2)], TORCH)
    populate(m, a, 4, 1, 1)
    return m, a, dict(dirt=6, bones=4, skull=2)


def magma_river():
    m = C.Map(34, 20, seed=42, fill=".", border="W")
    for _ in range(10):
        m.blob(m.rng.randint(2, 31), m.rng.randint(2, 17), m.rng.uniform(1.0, 2.2), "W", only=".")
    lava_river(m, (17, 0), (15, 19), [4, 11, 16])
    a = (3, 10)
    m.rect(1, 8, 5, 12, ".")
    m.set(*a, "A")
    heal(m, a)
    for _ in range(3):
        lava_pool(m, a, m.rng.randint(3, 30), m.rng.randint(3, 16), 1.2)
    populate(m, a, 6, 2, 2, (18, 1, 32, 18))
    populate(m, a, 2, 0, 1, (1, 1, 14, 18))
    return m, a, dict(dirt=10, sand=4, bones=6, skull=3)


def obsidian_maze():
    m = C.Map(33, 21, seed=43, border="W")
    m.maze(0, 0, 32, 20, "W", braid=1.0)
    for _ in range(25):  # knock out extra walls so spiders can be walked around
        x, y = m.rng.randrange(2, 31), m.rng.randrange(2, 19)
        if (x + y) % 2 == 1 and m.get(x, y) == "W":
            m.set(x, y, ".")
    a = (1, 1)
    m.set(*a, "A")
    for _ in range(14):  # lava fills some dead-end-free corridor cells
        p = m.rng.choice(m.free())
        if abs(p[0] - a[0]) + abs(p[1] - a[1]) > 4:
            safe_paint(m, a, [p], "L")
    m.rect(13, 8, 19, 12, ".")
    safe_paint(m, a, [(16, 10)], "N")
    populate(m, a, 7, 0, 2)
    return m, a, dict(dirt=8, bones=6, skull=4)


def fire_temple():
    m = C.Map(37, 22, seed=44, border="W")
    box = K.rooms(m, [0, 12, 24, 36], [0, 10, 21], wall="W", extra=0.6, door=2)
    a = K.centre(box[0][0])
    m.set(*a, "A")
    for row in box:
        for b in row:
            if b != box[0][0]:
                lava_pool(m, a, *K.centre(b), m.rng.uniform(1.4, 2.2))
            for x, y in ((b[0], b[1]), (b[2], b[1]), (b[0], b[3]), (b[2], b[3])):
                safe_paint(m, a, [(x, y)], TORCH)
    heal(m, a)
    populate(m, a, 8, 3, 2)
    return m, a, dict(dirt=6, sand=6, bones=8, skull=4)


def caldera():
    m = C.Map(38, 23, seed=45, fill=".", border="W")
    m.blob(19, 11, 7.0, "L", jitter=0.2)            # the lava lake
    m.blob(19, 11, 2.5, ".")                        # central island
    for p0, p1 in [((19, 4), (19, 9)), ((12, 11), (17, 11)), ((21, 11), (27, 11))]:
        m.walk(p0, p1, BRIDGE, width=1, wobble=0.0, only="L")
    for _ in range(12):
        m.blob(m.rng.randint(2, 35), m.rng.randint(2, 20), m.rng.uniform(0.8, 1.8), ROCK, only=".")
    a = (2, 2)
    m.rect(1, 1, 4, 4, ".")
    m.set(*a, "A")
    heal(m, a)
    populate(m, a, 3, 1, 0, (16, 8, 22, 14))
    populate(m, a, 6, 2, 4)
    return m, a, dict(dirt=10, sand=6, bones=6, skull=4)


def broodmother():
    m = C.Map(41, 25, seed=46, fill="W", border="W")
    hubs = [(3, 3), (12, 6), (6, 16), (17, 20), (20, 12), (28, 4), (31, 16), (14, 2), (36, 9), (37, 21)]
    cave(m, hubs, [(0, 1), (1, 4), (0, 2), (2, 3), (3, 4), (4, 5), (1, 7), (7, 5), (4, 6), (5, 8), (6, 9), (8, 6)])
    a = (3, 3)
    m.set(*a, "A")
    for _ in range(8):
        x, y = m.rng.choice(hubs[1:])
        lava_pool(m, a, x + m.rng.randint(-2, 2), y + m.rng.randint(-2, 2), m.rng.uniform(1.0, 1.8))
    heal(m, a)
    for _ in range(10):
        p = m.rng.choice(m.free())
        if any(m.get(p[0] + dx, p[1] + dy) == "W" for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            safe_paint(m, a, [p], TORCH)
    populate(m, a, 9, 4, 3)
    return m, a, dict(dirt=12, bones=14, skull=8, sand=3)


LEVELS = [
    ("Ember Hollow", "A small cave, a little lava and one nest.", ember_hollow),
    ("Magma River", "Cross the lava river by its log bridges.", magma_river),
    ("Obsidian Maze", "Twisting tunnels with lava in the corridors.", obsidian_maze),
    ("Fire Temple", "Six chambers, each with its own lava pit.", fire_temple),
    ("Caldera", "Gems on an island in a lava lake.", caldera),
    ("Broodmother", "Deep caves crawling with nests.", broodmother),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one gnome"
    seen = C.reach(rows, a[0], lambda c, p: c in WALK or c in "gS")
    gems = C.find(rows, "g")
    assert len(gems) >= 3, f"{name}: too few gems"
    missing = [p for p in gems if p not in seen]
    assert not missing, f"{name}: unreachable gems {missing}"
    ax, ay = a[0]
    near = [p for c in "SN" for p in C.find(rows, c) if abs(p[0] - ax) + abs(p[1] - ay) <= 6]
    assert not near, f"{name}: spider/nest within 6 of start {near}"
    return dict(gems=len(gems), nests=len(C.find(rows, "N")), spiders=len(C.find(rows, "S")),
                lava=len(C.find(rows, "L")))


# ----------------------------------------------------------------------------- GDY
OBJECTS = C.GROUND_NAMES + C.SCENERY_NAMES + ["wall", "lava", "gem", "nest", "spider", "gnome"]


def actions():
    me = "gnome"
    face = "{set_tile: meta.facing}"
    burn = "      - Src: {Object: spider, Commands: [{remove: true}]}\n        Dst: {Object: lava}\n"
    kill = "      - Src: {Object: spider}\n        Dst: {Object: gnome, Commands: [{remove: true}, {reward: -1}]}\n"
    return f'''
Actions:
{C.face_action(me)}
  # Spiders wander (rotate or step forwards at random), burn up in lava and kill the gnome.
{K.wander("random_movement", "spider", 3, OBJECTS, {"lava": burn, "gnome": kill}, relative=True)}
  # Nests hatch up to 3 spiders each into a free neighbouring cell.
{K.hatch("spawn_spider", "nest", "spider", 60, 15, OBJECTS, limit=3)}
  # The gnome walks up / down / left / right; bumping a gem collects it.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {me}, Commands: [{face}, {{reward: 1}}]}}
        Dst: {{Object: gem, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: [spider, lava]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: [{", ".join(C.SCENERY_NAMES)}, wall, nest]}}
'''


def objects():
    avatar = C.facing_sprites("oryx/oryx_fantasy/avatars/gnome1.png", "spider_nest-gnome")
    init = lambda a, d, extra="": f"{extra}    InitialActions:\n      - Action: {a}\n        Delay: {d}\n        Randomize: true\n"
    nest_vars = "    Variables:\n      - Name: hatched\n        InitialValue: 0\n"
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "W", 1, C.tiled("oryx/oryx_fantasy/wall8"), "[0.7, 0.7, 0.7]", "square", 1.0),
        C.obj("lava", "L", 1, C.img(C.TG + "lava.png"), "[1.0, 0.3, 0.0]", "square", 1.0),
        C.obj("gem", "g", 1, C.img("oryx/oryx_fantasy/ore-6.png"), "[0.0, 1.0, 0.0]", "triangle", 0.5),
        C.obj("nest", "N", 1, C.img("oryx/oryx_fantasy/bush-1.png"), "[0.0, 0.0, 1.0]", "triangle", 0.8,
              init("spawn_spider", 30, nest_vars)),
        C.obj("spider", "S", 2, C.img("oryx/oryx_fantasy/avatars/spider1.png"), "[1.0, 0.0, 0.0]", "triangle", 0.8,
              init("random_movement", 3)),
        C.obj("gnome", "A", 2, C.imgs(avatar), "[0.0, 0.0, 1.0]", "triangle", 0.8, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Spider Nest Remastered", """
        Collect every gem in the volcanic caves. Spiders wander at random and nests hatch more of them;
        touching a spider or stepping into lava is fatal (spiders burn in lava too). Cave walls, rocks
        and torches block the way; log bridges cross the lava.
        """.replace("        ", ""), "gnome", ["eq: [gem:count, 0]"], ["eq: [gnome:count, 0]"],
        background="oryx/oryx_fantasy/floor2-2.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid=SOLIDS)
        info = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9300 + i, stack_under="AgSN")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:14s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Spider Nest Remastered",
        "tagline": "Gems, lava and a lot of spiders.",
        "description": "Guide a gnome through volcanic caves, lava rivers and fire temples to collect every gem. "
                       "Nests keep hatching spiders, and one step into the lava is the end.",
        "tags": ["action", "exploration", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/spider-nest.yaml",
        "accent": "#f97316",
        "icon": "🕷️",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Gems left", "kind": "remaining", "object": "gem", "tone": "emerald"},
                {"label": "Spiders", "kind": "count", "object": "spider", "tone": "rose"}],
        "win_text": "Every gem collected!",
        "lose_text": "The caves claimed another gnome.",
    })


if __name__ == "__main__":
    main()
