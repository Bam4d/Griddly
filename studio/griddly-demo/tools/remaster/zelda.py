#!/usr/bin/env python3
"""Zelda Remastered: a sword-swinging hero crosses dungeons and overworlds, collecting keys, unlocking
doors and slaying spiders and bats on the way to the exit door.

Remaster of resources/games/Single-Player/GVGAI/zelda.yaml (one small room: grab the key, reach the
door, avoid or slash the wandering spiders). Same core rules: walking into a key picks it up, the exit
door only opens for a hero carrying a key (opening it wins), spiders kill on contact and `attack`
spawns a sword flash on the cell in front that kills a spider. Twists: locked doors (each uses up a
key) split the levels into regions, fast bats, and spider nests that hatch a few more spiders.

The hero no longer rotates: `move` is absolute (MOVE_INPUTS) and `attack` takes the same 4 inputs
(the UI sends the last move direction); both set the facing tile.

Layout characters (besides the shared terrain chars from common):
  W wall   A hero   k key   D locked door   E exit door   S spider   b bat   N spider nest   . floor
Every level is validated: rectangular, one hero, the exit reachable through doors opened with keys,
and an exhaustive door-order search proves no order of opening doors strands the hero without keys.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import action_kit as K  # noqa: E402
import common as C  # noqa: E402

GID = "zelda"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE, DIRT, SAND = G["bridge"], G["dirt"], G["sand"]
WALLS = "W" + HEDGE + STONE + ROCK
WALK = set(".") | set(G.values())
ENEMY = "SbN"


def enemies(m, start, spiders=0, bats=0, nests=0, region=None, min_dist=9):
    rows = K.grid_rows(m)
    r = C.reach(rows, start, lambda c, p: c in WALK or c in "kSbDE")
    for ch, n in (("S", spiders), ("b", bats)):
        for p in K.far_cells(m, start, n, r, min_dist, region):
            m.set(*p, ch)
    if nests:
        # nests need open space around them and must not seal anything off
        spots = [p for p in m.free(region) if p in r and abs(p[0] - start[0]) + abs(p[1] - start[1]) >= min_dist + 2
                 and sum(m.get(p[0] + dx, p[1] + dy) == "." for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))) == 4]
        m.rng.shuffle(spots)
        placed = 0
        for p in spots:
            if placed >= nests:
                break
            before = C.reach(K.grid_rows(m), start, lambda c, q: c in WALK or c in "kSbDE")
            m.set(*p, "N")
            after = C.reach(K.grid_rows(m), start, lambda c, q: c in WALK or c in "kSbDE")
            if len(after) != len(before) - 1:
                m.set(*p, ".")
                continue
            placed += 1


def props(m, start, ch, n, region=None, wall_side=False):
    """Scatter solid props that never disconnect reachable floor."""
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    for x, y in spots:
        if placed >= n:
            break
        if wall_side and not any(m.get(x + dx, y + dy) in WALLS for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            continue
        if (x, y) == start:
            continue
        ok = lambda c, p: c in WALK or c in "AkSbDE"
        before = C.reach(K.grid_rows(m), start, ok)
        m.set(x, y, ch)
        if len(C.reach(K.grid_rows(m), start, ok)) != len(before) - 1:
            m.set(x, y, ".")
            continue
        placed += 1


# ----------------------------------------------------------------------------- levels
def meadow_shrine():
    m = C.Map(27, 16, seed=1, border=TREE)
    m.blob(7, 11, 2.6, WATER)
    m.rect(16, 3, 24, 11, "W", filled=False)        # the shrine
    m.set(16, 7, "D")
    for p in [(18, 5), (22, 5), (18, 9), (22, 9)]:
        m.set(*p, TORCH)
    m.set(20, 7, "E")
    a = (2, 2)
    m.set(*a, "A")
    for cx, cy, r in [(11, 3, 1.4), (4, 6, 1.0), (12, 13, 1.3)]:
        m.blob(cx, cy, r, TREE, only=".")
    props(m, a, BUSH, 8, (1, 1, 14, 14))
    m.set(3, 13, "k")
    m.set(23, 10, "k")
    m.walk((1, 8), (15, 7), DIRT, width=1, wobble=0.1, only=".")
    enemies(m, a, spiders=3, region=(6, 1, 15, 14), min_dist=8)
    return m, a, dict(grass=12, moss=2, mush=4)


def old_keep():
    m = C.Map(31, 17, seed=2, border="W")
    box = K.rooms(m, [0, 10, 20, 30], [0, 8, 16], extra=0.25)
    a = K.centre(box[0][0])
    m.set(*a, "A")
    goal = (box[1][2][2] - 1, box[1][2][3] - 1)
    m.set(*goal, "E")
    K.lock_path(m, a, goal, 1, WALK, WALLS, min_key_dist=5)
    props(m, a, TORCH, 10, wall_side=True)
    props(m, a, TOMB, 3, box[0][1])
    enemies(m, a, spiders=4, bats=1)
    return m, a, dict(dirt=4, moss=3, bones=5, skull=2)


def forest_ruins():
    m = C.Map(36, 21, seed=3, border=TREE)
    m.walk((18, 0), (16, 20), WATER, width=2, wobble=0.35, only=".")
    for y in (5, 15):
        for x in range(1, 35):
            if m.get(x, y) == WATER:
                m.set(x, y, BRIDGE)
    m.rect(25, 3, 33, 12, STONE, filled=False)       # ruined temple
    m.rect(25, 7, 33, 7, STONE)
    m.set(29, 7, "D")
    m.set(25, 10, "D")
    m.set(29, 4, "E")
    for cx, cy, r in [(5, 5, 2.0), (10, 15, 2.2), (28, 17, 1.8), (22, 10, 1.4)]:
        m.blob(cx, cy, r, TREE, only=".")
    a = (2, 18)
    m.set(*a, "A")
    props(m, a, BUSH, 14)
    m.set(32, 18, "k")
    m.set(3, 2, "k")
    m.set(31, 11, "k")
    enemies(m, a, spiders=5, bats=1, nests=1, min_dist=10)
    return m, a, dict(grass=18, moss=4, mush=6, dirt=3)


def catacombs():
    m = C.Map(37, 23, seed=4, border=STONE, fill=".")
    box = K.rooms(m, [0, 12, 24, 36], [0, 7, 15, 22], wall=STONE, extra=0.3)
    a = K.centre(box[0][0])
    m.set(*a, "A")
    goal = K.centre(box[2][2])
    m.set(*goal, "E")
    K.lock_path(m, a, goal, 2, WALK, WALLS, min_key_dist=5)
    for row in box:
        for b in row:
            for x in range(b[0] + 1, b[2], 3):
                if m.rng.random() < 0.5:
                    props(m, a, TOMB, 1, (x, b[1] + 1, x, b[1] + 1))
    props(m, a, TORCH, 14, wall_side=True)
    enemies(m, a, spiders=5, bats=2, nests=2)
    return m, a, dict(dirt=8, moss=4, bones=14, skull=8)


def lake_temple():
    m = C.Map(39, 23, seed=5, border=ROCK)
    box = K.rooms(m, [0, 13, 26, 38], [0, 11, 22], wall=ROCK, extra=0.4)
    a = (2, 2)
    m.set(*a, "A")
    goal = (box[1][1][0] + 1, box[1][1][3] - 1)
    m.set(*goal, "E")
    K.lock_path(m, a, goal, 2, WALK, WALLS, min_key_dist=6)
    for row in box:
        for b in row:
            cx, cy = K.centre(b)
            cells = [(x, y) for y in range(cy - 3, cy + 4) for x in range(cx - 4, cx + 5)
                     if ((x - cx) ** 2 / 16 + (y - cy) ** 2 / 6) <= 1 and m.get(x, y) == "."]
            for p in cells:
                props(m, a, WATER, 1, (*p, *p))
    props(m, a, BUSH, 8)
    props(m, a, TORCH, 8, wall_side=True)
    enemies(m, a, spiders=5, bats=3, nests=1)
    return m, a, dict(moss=12, grass=6, mush=6, sand=4)


def dark_citadel():
    m = C.Map(41, 25, seed=6, border="W")
    box = K.rooms(m, [0, 9, 18, 27, 40], [0, 8, 16, 24], extra=0.3)
    a = K.centre(box[2][0])
    m.set(*a, "A")
    goal = K.centre(box[0][3])
    m.set(*goal, "E")
    K.lock_path(m, a, goal, 3, WALK, WALLS, min_key_dist=5)
    hall = box[1][1]
    for x in range(hall[0] + 1, hall[2], 2):
        props(m, a, STONE, 1, (x, hall[1] + 1, x, hall[1] + 1))
    props(m, a, TORCH, 16, wall_side=True)
    props(m, a, TOMB, 4, box[2][2])
    props(m, a, ROCK, 6)
    enemies(m, a, spiders=7, bats=3, nests=2)
    return m, a, dict(dirt=10, bones=12, skull=8, moss=3)


LEVELS = [
    ("Meadow Shrine", "Find a key in the meadow and open the shrine.", meadow_shrine),
    ("Old Keep", "Six rooms, one locked door. Spiders roam the halls.", old_keep),
    ("Forest Ruins", "Cross the river to the ruined temple. A nest hatches in the woods.", forest_ruins),
    ("Catacombs", "Nine crypts and two locked doors. Bats flit between the graves.", catacombs),
    ("Lake Temple", "Flooded chambers behind rock walls.", lake_temple),
    ("Dark Citadel", "Twelve rooms, three locks, nests in the dark. Good luck, hero.", dark_citadel),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one hero"
    assert len(C.find(rows, "E")) == 1, f"{name}: need one exit"
    try:
        states, doors = K.check_keys(rows, WALK | set("Sb"), goal="E")
    except AssertionError as e:
        raise AssertionError(f"{name}: {e}") from None
    ax, ay = a[0]
    near = [p for c in ENEMY for p in C.find(rows, c) if abs(p[0] - ax) + abs(p[1] - ay) <= 5]
    assert not near, f"{name}: enemy within 5 of start {near}"
    return dict(keys=len(C.find(rows, "k")), doors=doors, spiders=len(C.find(rows, "S")),
                bats=len(C.find(rows, "b")), nests=len(C.find(rows, "N")), door_states=states)


# ----------------------------------------------------------------------------- GDY
OBJECTS = (C.GROUND_NAMES + C.SCENERY_NAMES + ["wall", "key", "door", "exit", "nest", "spider", "bat", "avatar",
                                              "attack_fire"])


def die(mover, victim="avatar"):
    return (f"      - Src: {{Object: {mover}}}\n"
            f"        Dst: {{Object: {victim}, Commands: [{{remove: true}}, {{reward: -1}}]}}\n")


def burn(mover):
    return (f"      - Src: {{Object: {mover}, Commands: [{{remove: true}}]}}\n"
            f"        Dst: {{Object: attack_fire}}\n")


def actions():
    special = lambda mover: {"avatar": die(mover), "attack_fire": burn(mover)}
    hero = "avatar"
    face = "{set_tile: meta.facing}"
    return f'''
Actions:
{C.face_action(hero)}
  # Spiders wander slowly, bats quickly (absolute random steps). Touching the hero kills it; walking
  # into a sword flash kills them. Anything else blocks them and they try again.
{K.wander("spider_move", "spider", 5, OBJECTS, special("spider"))}
{K.wander("bat_move", "bat", 2, OBJECTS, special("bat"))}
  # Nests hatch up to 3 spiders, one every ~60 ticks, into a free neighbouring cell.
{K.hatch("hatch", "nest", "spider", 60, 15, OBJECTS, limit=3)}
  # The sword flash fades after a few ticks.
  - Name: fade
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Fade, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: attack_fire, Commands: [{{remove: true}}]}}
        Dst: {{Object: attack_fire}}

  # The hero walks up / down / left / right (absolute; it never rotates).
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {hero}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {hero}, Commands: [{face}, {{mov: _dest}}, {{incr: keys}}, {{reward: 1}}]}}
        Dst: {{Object: key, Commands: [{{remove: true}}]}}
      - Src:
          Object: {hero}
          Preconditions: [{{gte: [src.keys, 1]}}]
          Commands: [{face}, {{decr: keys}}]
        Dst: {{Object: door, Commands: [{{remove: true}}]}}
      - Src:
          Object: {hero}
          Preconditions: [{{gte: [src.keys, 1]}}]
          Commands: [{face}, {{decr: keys}}, {{reward: 5}}]
        Dst: {{Object: exit, Commands: [{{remove: true}}]}}
      - Src: {{Object: {hero}, Commands: [{{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: [spider, bat]}}
      - Src: {{Object: {hero}, Commands: [{face}]}}
        Dst: {{Object: [{", ".join(C.SCENERY_NAMES)}, wall, nest, attack_fire]}}

  # Swing the sword at the cell in the given direction (the UI sends the last move direction).
  - Name: attack
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {hero}, Commands: [{face}, {{spawn: attack_fire}}, {{reward: 1}}]}}
        Dst: {{Object: [spider, bat], Commands: [{{remove: true}}]}}
      - Src: {{Object: {hero}, Commands: [{face}, {{spawn: attack_fire}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {hero}, Commands: [{face}]}}
        Dst: {{Object: [{", ".join(C.SCENERY_NAMES)}, wall, nest, key, door, exit, attack_fire]}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/oryx/swordman1_0.png", "zelda-avatar")
    init = lambda a, d: f"    InitialActions:\n      - Action: {a}\n        Delay: {d}\n        Randomize: true\n"
    hero_extra = "    Variables:\n      - Name: keys\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION
    nest_extra = "    Variables:\n      - Name: hatched\n        InitialValue: 0\n" + init("hatch", 40)
    fire_extra = "    InitialActions:\n      - Action: fade\n        ActionId: 1\n        Delay: 3\n"
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "W", 1, C.tiled("gvgai/oryx/wall3", "_"), "[0.7, 0.7, 0.7]", "square", 1.0),
        C.obj("key", "k", 1, C.img("gvgai/oryx/key2.png"), "[1.0, 0.85, 0.1]", "triangle", 0.6),
        C.obj("door", "D", 1, C.img("oryx/oryx_fantasy/door-1.png"), "[0.6, 0.4, 0.1]", "square", 0.9),
        C.obj("exit", "E", 1, C.img("gvgai/oryx/doorclosed1.png"), "[0.0, 0.7, 0.0]", "square", 0.8),
        C.obj("nest", "N", 1, C.img("gvgai/newset/cocoonb1.png"), "[0.6, 0.6, 0.5]", "circle", 0.7, nest_extra),
        C.obj("spider", "S", 2, C.img("oryx/oryx_fantasy/avatars/spider1.png"), "[0.9, 0.1, 0.1]", "triangle", 0.6,
              init("spider_move", 5)),
        C.obj("bat", "b", 2, C.img("gvgai/oryx/bat1.png"), "[0.6, 0.1, 0.7]", "triangle", 0.6, init("bat_move", 2)),
        C.obj("avatar", "A", 2, C.imgs(avatar), "[0.0, 0.5, 0.5]", "triangle", 0.9, hero_extra, tiles=4),
        C.obj("attack_fire", None, 3, C.img("gvgai/oryx/fire1.png"), "[1.0, 0.3, 0.0]", "square", 0.5, fire_extra),
    ])


def main():
    parts = [C.header("Zelda Remastered", """
        Grab keys, unlock doors and reach the exit door with a key in hand. Spiders and bats wander at
        random and kill on contact; swing your sword (attack) at the cell in front of you to slay them.
        Nests hatch more spiders. Walls, water, trees, bushes, tombstones and torches block the way.
        """.replace("        ", ""), "avatar", ["eq: [exit:count, 0]"], ["eq: [avatar:count, 0]"],
        background="gvgai/newset/floor2.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid=WALLS + TREE)
        info = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9100 + i, stack_under="AkSbDEN")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:14s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Zelda Remastered",
        "tagline": "Keys, locked doors and a sword against the spiders.",
        "description": "Explore meadows, keeps, forest ruins, catacombs and a dark citadel. Collect keys to "
                       "open locked doors and the exit, and slash the spiders and bats that roam the rooms.",
        "tags": ["action", "exploration", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/zelda.yaml",
        "accent": "#22c55e",
        "icon": "🗡️",
        "controls": {"move": "move", "extra": [{"key": " ", "label": "Attack", "action": "attack",
                                                "directional": True}]},
        "hud": [{"label": "Keys", "kind": "var", "object": "avatar", "variable": "keys", "tone": "amber"},
                {"label": "Spiders", "kind": "count", "object": "spider", "tone": "rose"},
                {"label": "Bats", "kind": "count", "object": "bat", "tone": "fuchsia"}],
        "win_text": "The exit door swings open. Well done, hero!",
        "lose_text": "Slain in the dungeon.",
    })


if __name__ == "__main__":
    main()
