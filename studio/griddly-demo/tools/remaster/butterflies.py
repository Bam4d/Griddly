#!/usr/bin/env python3
"""Butterflies Remastered: catch every butterfly in gardens, meadows, orchards and marshes before the
spiders eat them - and before the spiders eat you.

Remaster of resources/games/Single-Player/GVGAI/butterflies.yaml. Same core rules: butterflies
flutter at random, cocoons hatch new butterflies, spiders wander (rotating at random), eat any
butterfly they walk into and kill the catcher on contact; walking into a butterfly catches it (+1).
Changes: each cocoon hatches exactly one butterfly (at a random time) and the level is won when no
butterflies AND no cocoons are left (the original's cocoons hatched forever); a butterfly that
flutters into the catcher is caught too.

Layout characters (besides the shared terrain chars from common):
  A catcher   f butterfly   c cocoon   S spider   . floor (lawn background)
Every level is validated: rectangular, one catcher, every butterfly reachable, every cocoon has a free
neighbour reachable by the catcher, and no spider within 7 tiles of the start.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import action_kit as K  # noqa: E402
import common as C  # noqa: E402

GID = "butterflies"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE, DIRT, SAND = G["bridge"], G["dirt"], G["sand"]
WALK = set(".") | set(G.values())
SOLIDS = HEDGE + TREE + STONE + ROCK + WATER


def populate(m, a, butterflies, cocoons, spiders, region=None):
    rows = K.grid_rows(m)
    r = C.reach(rows, a, lambda c, p: c in WALK or c in "fcS")
    for p in K.far_cells(m, a, butterflies, r, 4, region):
        m.set(*p, "f")
    spots = [p for p in m.free(region) if p in r and abs(p[0] - a[0]) + abs(p[1] - a[1]) >= 5
             and sum(m.get(p[0] + dx, p[1] + dy) == "." for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))) >= 3]
    m.rng.shuffle(spots)
    for p in spots[:cocoons]:
        m.set(*p, "c")
    for p in K.far_cells(m, a, spiders, r, 10, region):
        m.set(*p, "S")


def safe_props(m, a, ch, n, region=None):
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    ok = lambda c, p: c in WALK or c in "AfcS"
    for p in spots:
        if placed >= n:
            break
        before = C.reach(K.grid_rows(m), a, ok)
        m.set(*p, ch)
        if len(C.reach(K.grid_rows(m), a, ok)) != len(before) - 1:
            m.set(*p, ".")
            continue
        placed += 1


# ----------------------------------------------------------------------------- levels
def cottage_garden():
    m = C.Map(27, 16, seed=31, border=HEDGE)
    m.blob(19, 10, 2.8, WATER)
    for y in (3, 12):                               # flower beds
        m.rect(3, y, 10, y, BUSH)
        m.set(m.rng.randint(4, 9), y, ".")
    m.rect(12, 2, 15, 5, STONE, filled=False)       # garden shed
    m.set(13, 5, ".")
    for p in [(24, 2), (24, 13), (2, 8)]:
        m.set(*p, TREE)
    m.walk((1, 8), (25, 6), SAND, width=1, wobble=0.15, only=".")
    a = (5, 8)
    m.set(*a, "A")
    populate(m, a, 5, 4, 2)
    return m, a, dict(grass=10, moss=2, mush=4)


def wildflower_meadow():
    m = C.Map(35, 20, seed=32, border=TREE)
    for cx, cy, r in [(6, 4, 1.8), (28, 15, 2.2), (30, 4, 1.5), (14, 15, 1.4), (20, 6, 1.2)]:
        m.blob(cx, cy, r, TREE, only=".")
    m.blob(11, 9, 2.0, WATER, only=".")
    a = (17, 10)
    m.set(*a, "A")
    safe_props(m, a, BUSH, 22)
    m.walk((1, 17), (33, 2), DIRT, width=1, wobble=0.2, only=".")
    populate(m, a, 7, 6, 3)
    return m, a, dict(grass=22, moss=3, mush=8, sand=2)


def hedge_maze():
    m = C.Map(33, 21, seed=33, border=HEDGE)
    m.maze(0, 0, 32, 20, HEDGE, braid=0.85)
    m.rect(12, 7, 20, 13, ".")
    m.rect(14, 9, 18, 11, WATER)
    m.set(16, 9, BRIDGE)
    for p in [(12, 7), (20, 7), (12, 13), (20, 13)]:
        m.set(*p, TREE)
    a = (16, 8)
    m.set(*a, "A")
    populate(m, a, 7, 5, 4)
    return m, a, dict(grass=12, dirt=4, mush=5)


def orchard():
    m = C.Map(37, 22, seed=34, border=HEDGE)
    m.walk((25, 0), (23, 21), WATER, width=1, wobble=0.25, only=".")
    for y in (4, 11, 18):
        for x in range(1, 36):
            if m.get(x, y) == WATER:
                m.set(x, y, BRIDGE)
    for y in range(3, 20, 3):                       # rows of fruit trees
        for x in range(3, 22, 3):
            if m.rng.random() < 0.85:
                m.set(x, y, TREE)
    m.rect(28, 3, 34, 9, STONE, filled=False)       # walled beehive yard
    m.set(28, 6, ".")
    a = (2, 20)
    m.set(*a, "A")
    safe_props(m, a, BUSH, 10, (26, 10, 35, 20))
    populate(m, a, 8, 7, 4)
    return m, a, dict(grass=18, dirt=4, mush=6, sand=2)


def marsh():
    m = C.Map(38, 22, seed=35, border=WATER)
    for _ in range(16):
        m.blob(m.rng.randint(3, 34), m.rng.randint(3, 18), m.rng.uniform(1.0, 2.4), WATER, only=".")
    a = (3, 3)
    m.rect(1, 1, 6, 6, ".")
    m.set(*a, "A")
    reach = C.reach(K.grid_rows(m), a, lambda c, p: c in WALK)
    for y in range(1, m.h - 1):                    # bridge every pond that cuts off dry land
        for x in range(1, m.w - 1):
            if m.get(x, y) == WATER and (x, y) not in reach and False:
                pass
    # connect stray dry patches with log bridges
    while True:
        rows = K.grid_rows(m)
        reach = C.reach(rows, a, lambda c, p: c in WALK)
        dry = [p for p in m.free() if p not in reach]
        if not dry:
            break
        tgt = min(dry, key=lambda p: min(abs(p[0] - q[0]) + abs(p[1] - q[1]) for q in reach))
        src = min(reach, key=lambda q: abs(tgt[0] - q[0]) + abs(tgt[1] - q[1]))
        x, y = src
        while (x, y) != tgt:
            if x != tgt[0]:
                x += (tgt[0] > x) - (tgt[0] < x)
            else:
                y += (tgt[1] > y) - (tgt[1] < y)
            if m.get(x, y) == WATER:
                m.set(x, y, BRIDGE)
    safe_props(m, a, TREE, 12)
    safe_props(m, a, BUSH, 10)
    populate(m, a, 8, 7, 5)
    return m, a, dict(moss=18, grass=10, mush=8)


def conservatory():
    m = C.Map(41, 25, seed=36, border=HEDGE)
    box = K.rooms(m, [0, 13, 27, 40], [0, 8, 16, 24], wall=HEDGE, extra=0.5, door=2)
    a = K.centre(box[1][1])
    m.set(*a, "A")
    for row in box:
        for b in row:
            if b == box[1][1]:
                continue
            if m.rng.random() < 0.5:
                cx, cy = K.centre(b)
                safe_props(m, a, WATER, 4, (cx - 2, cy - 1, cx + 2, cy + 1))
            safe_props(m, a, TREE, 2, b)
            safe_props(m, a, BUSH, 3, b)
    for b in (box[0][0], box[0][2], box[2][0], box[2][2], box[0][1], box[2][1]):
        populate(m, a, 2, 2, 0, b)
    populate(m, a, 0, 0, 6)
    return m, a, dict(grass=20, sand=4, dirt=3, mush=6)


LEVELS = [
    ("Cottage Garden", "Flower beds, a pond and a shed. Catch the first flutter.", cottage_garden),
    ("Wildflower Meadow", "An open meadow full of bushes and hungry spiders.", wildflower_meadow),
    ("Hedge Maze", "Butterflies flit through a looping hedge maze.", hedge_maze),
    ("Orchard", "Rows of fruit trees, a stream and a walled yard.", orchard),
    ("The Marsh", "Islands of reeds joined by log bridges.", marsh),
    ("Conservatory", "Nine hedge rooms, each with its own cocoons.", conservatory),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one catcher"
    seen = C.reach(rows, a[0], lambda c, p: c in WALK or c in "fS")
    flies, cocoons = C.find(rows, "f"), C.find(rows, "c")
    assert flies, f"{name}: no butterflies"
    missing = [p for p in flies if p not in seen]
    assert not missing, f"{name}: unreachable butterflies {missing}"
    stuck = [p for p in cocoons if not any((p[0] + dx, p[1] + dy) in seen for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))]
    assert not stuck, f"{name}: cocoons that hatch out of reach {stuck}"
    ax, ay = a[0]
    near = [p for p in C.find(rows, "S") if abs(p[0] - ax) + abs(p[1] - ay) <= 7]
    assert not near, f"{name}: spider within 7 of start {near}"
    return dict(butterflies=len(flies), cocoons=len(cocoons), spiders=len(C.find(rows, "S")))


# ----------------------------------------------------------------------------- GDY
OBJECTS = C.GROUND_NAMES + C.SCENERY_NAMES + ["cocoon", "butterfly", "spider", "catcher"]


def actions():
    me = "catcher"
    face = "{set_tile: meta.facing}"
    eat = ("      - Src: {Object: spider, Commands: [{mov: _dest}, {exec: {Action: spider_move, Delay: 3, Randomize: true}}]}\n"
           "        Dst: {Object: butterfly, Commands: [{remove: true}]}\n")
    kill = "      - Src: {Object: spider}\n        Dst: {Object: catcher, Commands: [{remove: true}, {reward: -1}]}\n"
    caught = ("      - Src: {Object: butterfly, Commands: [{remove: true}]}\n"
              "        Dst: {Object: catcher, Commands: [{reward: 1}]}\n")
    return f'''
Actions:
{C.face_action(me)}
  # Butterflies flutter at random (absolute steps); flying into the catcher gets them caught.
{K.wander("butterfly_move", "butterfly", 3, OBJECTS, {"catcher": caught})}
  # Spiders wander (rotate or step forwards at random), eat butterflies and kill the catcher.
{K.wander("spider_move", "spider", 3, OBJECTS, {"butterfly": eat, "catcher": kill}, relative=True)}
  # Each cocoon hatches one butterfly into a free neighbouring cell at a random time.
{K.hatch("hatch", "cocoon", "butterfly", 0, 10, OBJECTS, consume=True, idle=16)}
  # The catcher runs up / down / left / right; running into a butterfly catches it.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{reward: 1}}]}}
        Dst: {{Object: butterfly, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: spider}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: [{", ".join(C.SCENERY_NAMES)}, cocoon]}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/newset/girl5.png", "butterflies-catcher")
    init = lambda a, d, extra="": f"    InitialActions:\n      - Action: {a}\n        Delay: {d}\n        Randomize: true\n{extra}"
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("cocoon", "c", 1, C.img("gvgai/newset/cocoonb1.png"), "[0.0, 1.0, 0.0]", "triangle", 0.5,
              init("hatch", 20)),
        C.obj("butterfly", "f", 2, C.img("gvgai/newset/butterfly1.png"), "[0.0, 0.0, 1.0]", "triangle", 0.4,
              init("butterfly_move", 3)),
        C.obj("spider", "S", 2, C.img("oryx/oryx_fantasy/avatars/spider1.png"), "[1.0, 0.0, 0.0]", "triangle", 0.6,
              init("spider_move", 3)),
        C.obj("catcher", "A", 2, C.imgs(avatar), "[1.0, 1.0, 1.0]", "triangle", 0.8, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Butterflies Remastered", """
        Catch every butterfly! Butterflies flutter at random and cocoons hatch new ones; spiders wander
        the gardens eating butterflies - and catchers. You win when no butterflies or cocoons are left.
        Hedges, trees, bushes and water block the way; lawns, paths and bridges are walkable.
        """.replace("        ", ""), "catcher",
        ["and: [{eq: [butterfly:count, 0]}, {eq: [cocoon:count, 0]}]"], ["eq: [catcher:count, 0]"],
        background="gvgai/oryx/grass_15.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid=SOLIDS)
        info = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9200 + i, stack_under="AfcS")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:17s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Butterflies Remastered",
        "tagline": "Catch every butterfly before the spiders do.",
        "description": "Chase butterflies through cottage gardens, meadows, hedge mazes, orchards and marshes. "
                       "Cocoons hatch more of them, and spiders hunt them - and you.",
        "tags": ["action", "arcade", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/butterflies.yaml",
        "accent": "#38bdf8",
        "icon": "🦋",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Butterflies", "kind": "count", "object": "butterfly", "tone": "sky"},
                {"label": "Cocoons", "kind": "count", "object": "cocoon", "tone": "emerald"},
                {"label": "Spiders", "kind": "count", "object": "spider", "tone": "rose"}],
        "win_text": "Every butterfly caught!",
        "lose_text": "Caught by a spider.",
    })


if __name__ == "__main__":
    main()
