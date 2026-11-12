#!/usr/bin/env python3
"""Doggo Remastered: a doggo roams big parks and gardens fetching every stick.

Remaster of resources/games/Single-Player/Mini-Grid/minigrid-doggo.yaml (one stick in an empty room).
Here every level hides several sticks across ponds, hedges, woods and hedge mazes; the level is won
when no stick is left. Same core rule as the original: bumping a stick picks it up (+1).

Layout characters (besides the shared terrain chars from common):
  A doggo   g stick   . floor (park lawn background)
Every level is validated: rectangular, one doggo, every stick reachable (sticks count as passable,
since fetching one clears its cell).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

GID = "doggo"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE, SAND, DIRT = G["bridge"], G["sand"], G["dirt"]
WALKABLE = set(".") | set(G.values())


# ----------------------------------------------------------------------------- levels
def connect4(m, chars, fill):
    """Map.walk may step diagonally; open one orthogonal corner wherever two `chars` cells touch only
    diagonally, so trails are walkable with 4-way movement."""
    for y in range(1, m.h - 2):
        for x in range(1, m.w - 2):
            for dx in (1, -1):
                a, b = m.get(x, y), m.get(x + dx, y + 1) if 0 <= x + dx < m.w else None
                if a in chars and b in chars and m.get(x + dx, y) not in chars and m.get(x, y + 1) not in chars:
                    m.set(x + dx, y, fill) if m.rng.random() < 0.5 else m.set(x, y + 1, fill)


def back_garden():
    m = C.Map(26, 16, seed=101, border=HEDGE)
    m.blob(18, 10, 3.0, WATER)                      # garden pond
    m.rect(4, 3, 9, 3, BUSH)                        # flower beds
    m.rect(4, 12, 9, 12, BUSH)
    m.set(6, 3, ".")
    for p in [(13, 2), (23, 2), (2, 7), (13, 13), (23, 13)]:
        m.set(*p, TREE)
    m.walk((1, 8), (24, 4), SAND, width=1, wobble=0.2, only=".")   # garden path
    m.rect(11, 6, 13, 9, STONE, filled=False)       # little shed
    m.set(12, 9, ".")
    m.set(2, 2, "A")
    for p in [(12, 7), (21, 13), (6, 2), (23, 8)]:
        m.set(*p, "g")
    return m, dict(dirt=3, moss=2, mush=3)


def duck_pond():
    m = C.Map(34, 20, seed=202, border=TREE)
    m.blob(17, 10, 6.0, WATER, jitter=0.25)         # the big pond
    m.rect(15, 9, 19, 11, ".")                      # island
    m.blob(17, 10, 1.6, ".")
    for x in range(8, 16):                          # bridge from the west
        if m.get(x, 10) == WATER:
            m.set(x, 10, BRIDGE)
    for y in range(2, 9):                           # bridge from the north
        if m.get(17, y) == WATER:
            m.set(17, y, BRIDGE)
    m.set(17, 10, TREE)
    for cx, cy, r in [(4, 4, 1.8), (29, 15, 2.2), (30, 4, 1.5), (5, 16, 1.6)]:
        m.blob(cx, cy, r, TREE, only=".")
    m.scatter(BUSH, 16)
    m.walk((1, 18), (32, 1), SAND, width=1, wobble=0.15, only=".")
    m.set(2, 17, "A")
    for p in [(16, 9), (31, 2), (27, 17), (3, 2), (32, 10)]:
        m.set(*p, "g")
    return m, dict(dirt=4, moss=3, mush=6)


def hedge_maze():
    m = C.Map(33, 21, seed=303, border=HEDGE)
    m.maze(0, 0, 32, 20, HEDGE, braid=0.7)
    m.rect(12, 7, 20, 13, ".")                      # central lawn
    m.rect(14, 9, 18, 11, WATER)                    # fountain
    m.set(16, 9, BRIDGE)
    for p in [(12, 7), (20, 7), (12, 13), (20, 13)]:
        m.set(*p, TREE)
    m.set(1, 1, "A")
    for p in [(31, 1), (1, 19), (31, 19), (16, 8), (16, 12)]:
        m.set(*p, "g")
    return m, dict(dirt=6, sand=2, mush=4)


def woodland_walk():
    m = C.Map(38, 22, seed=404, fill=TREE, border=TREE)
    hubs = [(3, 3), (12, 5), (7, 15), (18, 18), (20, 10), (28, 4), (33, 13), (31, 19), (14, 2)]
    for a, b in [(0, 1), (1, 4), (0, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (3, 7), (1, 8), (8, 5)]:
        m.walk(hubs[a], hubs[b], DIRT, width=1, wobble=0.35)
    connect4(m, {DIRT}, DIRT)
    for x, y in hubs:
        m.blob(x, y, m.rng.uniform(1.8, 2.8), ".")
    m.walk((24, 1), (26, 20), WATER, width=1, wobble=0.3, only=(TREE,))  # a stream in the woods
    m.scatter(BUSH, 10, clear=3)
    m.set(3, 3, "A")
    for p in [(12, 5), (7, 16), (18, 18), (28, 4), (33, 13), (31, 19), (14, 2)]:
        m.set(*p, "g")
    return m, dict(moss=10, grass=6, mush=10, bones=2)


def riverside():
    m = C.Map(40, 24, seed=505, border=HEDGE)
    m.walk((20, 0), (17, 23), WATER, width=2, wobble=0.4, only=".")
    for y in (4, 12, 20):                           # three bridges
        for x in range(1, 39):
            if m.get(x, y) == WATER:
                m.set(x, y, BRIDGE)
    m.maze(25, 13, 37, 21, HEDGE, braid=0.5)        # a small hedge maze on the east bank
    m.set(25, 15, ".")
    m.set(25, 17, ".")
    m.rect(26, 2, 36, 9, STONE, filled=False)       # walled orchard
    m.set(26, 6, ".")
    for x in range(28, 36, 2):
        for y in (4, 7):
            m.set(x, y, TREE)
    for cx, cy, r in [(5, 5, 2.0), (9, 18, 2.4)]:
        m.blob(cx, cy, r, WATER, only=".")
    m.scatter(TREE, 10, (1, 1, 16, 22), clear=3)
    m.scatter(BUSH, 12, clear=3)
    m.set(2, 21, "A")
    for p in [(34, 20), (31, 5), (35, 8), (2, 2), (12, 12), (23, 1), (27, 11)]:
        m.set(*p, "g")
    return m, dict(dirt=6, sand=4, moss=4, mush=6)


LEVELS = [
    ("Back Garden", "A walled garden, a pond and a shed. Fetch all four sticks.", back_garden),
    ("Duck Pond", "Cross the bridges to the island in the middle of the park.", duck_pond),
    ("Hedge Maze", "Sticks are hidden in the corners of a looping hedge maze.", hedge_maze),
    ("Woodland Walk", "Follow the winding dirt trails through the woods.", woodland_walk),
    ("Riverside", "Bridges, an orchard and a little maze across the river.", riverside),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one doggo"
    sticks = C.find(rows, "g")
    assert len(sticks) >= 3, f"{name}: too few sticks"
    seen = C.reach(rows, a[0], lambda c, p: c in WALKABLE or c == "g")
    missing = [p for p in sticks if p not in seen]
    assert not missing, f"{name}: unreachable sticks {missing}"
    return len(sticks)


# ----------------------------------------------------------------------------- GDY
ACTIONS = f'''
Actions:
{C.face_action("doggo")}
  # The doggo runs up / down / left / right; bumping a stick picks it up.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: doggo, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: doggo, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}]}}
        Dst: {{Object: stick, Commands: [{{remove: true}}]}}
      - Src: {{Object: doggo, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: {C.SCENERY_LIST}}}
'''


def objects():
    avatar = C.facing_sprites("oryx/oryx_fantasy/avatars/doggo1.png", "doggo-doggo")
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("stick", "g", 1, C.img("oryx/oryx_fantasy/stick-0.png"), "[0.0, 1.0, 0.0]", "triangle", 0.6),
        C.obj("doggo", "A", 2, C.imgs(avatar), "[1.0, 0.0, 0.0]", "triangle", 0.9, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Doggo Remastered", """
        Doggo is off to the park! Fetch every stick hidden among ponds, hedges, woods and hedge mazes.
        Water, trees, bushes and hedges block the way; lawns, paths and bridges are walkable.
        You win when no sticks are left.
        """.replace("        ", ""), "doggo", ["eq: [stick:count, 0]"], [],
        background="gvgai/oryx/grass_15.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, recipe = build()
        rows = m.rows(solid=HEDGE + TREE)
        n = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=7000 + i, stack_under="Ag")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:15s} {len(rows[0])}x{len(rows)} sticks={n}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    C.emit(GID, gdy, {
        "name": "Doggo Remastered",
        "tagline": "Fetch every stick in the park.",
        "description": "A good doggo explores gardens, duck ponds, hedge mazes and woodland trails "
                       "to fetch every stick. Bridges cross the water; hedges and trees block the way.",
        "tags": ["exploration", "maze", "classic-port"],
        "difficulty": "easy",
        "original": "Mini-Grid/minigrid-doggo.yaml",
        "accent": "#84cc16",
        "icon": "🐕",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Sticks left", "kind": "remaining", "object": "stick", "tone": "emerald"}],
        "win_text": "Good doggo! Every stick fetched.",
        "lose_text": "Out of time.",
    })


if __name__ == "__main__":
    main()
