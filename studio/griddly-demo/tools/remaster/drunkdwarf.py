#!/usr/bin/env python3
"""Drunk Dwarf Remastered: stumble through taverns, inns and cellars, collecting keys to unlock door
after door until you reach your (coffin) bed.

Remaster of resources/games/Single-Player/Mini-Grid/minigrid-drunkdwarf.yaml. Same core rules: walk
into a key to pick it up, walk into a locked door while holding a key to unlock it (it becomes an
open doorway), reach the coffin bed to win. Furniture (tables, chairs, bookshelves, barrels) and a
snoozing doggo block the way. The original relative `stumble` action is now the absolute `move`.

Key rule (consistent across levels): every key opens any one door and is CONSUMED doing so
(`keys` counter on the dwarf). Levels chain several doors; some keys sit behind doors.

Layout characters (besides the shared terrain chars from common):
  W wall  D locked door  k key  g coffin bed  t table  c chair  b bookshelf  B barrel  d doggo
  A dwarf  . floor
Validation: rectangular, one dwarf, one coffin, keys >= doors, and an exhaustive search over
(keys picked, doors opened) states proving the coffin is reachable from the start AND from every
reachable state (no softlock however the keys are spent).
"""
from __future__ import annotations

import sys
from functools import lru_cache
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

GID = "drunkdwarf"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE = G["bridge"]
FLOOR = set(".A") | set(G.values())
WALLS = "W" + ROCK + STONE
DIRS = ((1, 0), (-1, 0), (0, 1), (0, -1))


# ----------------------------------------------------------------------------- helpers
def conn(m, start):
    return C.reach(["".join(r) for r in m.g], start, lambda c, p: c in FLOOR or c in "Dkg")


def furnish(m, start, cells, ch):
    """Paint cells (all must be floor) with ch unless that would cut off any floor/door/key."""
    if any(not m.inside(*p) or m.get(*p) != "." or p == start for p in cells):
        return False
    before = conn(m, start)
    for p in cells:
        m.set(*p, ch)
    if len(conn(m, start)) != len(before) - len(cells):
        for p in cells:
            m.set(*p, ".")
        return False
    return True


def dining(m, start, region, n):
    """n tables with chairs either side."""
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    for x, y in spots:
        if placed >= n:
            break
        if furnish(m, start, [(x, y)], "t"):
            placed += 1
            for dx in (-1, 1):
                if m.rng.random() < 0.85:
                    furnish(m, start, [(x + dx, y)], "c")


def along_walls(m, start, region, ch, n):
    spots = [p for p in m.free(region) if any(m.get(p[0] + dx, p[1] + dy) in WALLS for dx, dy in DIRS)]
    m.rng.shuffle(spots)
    placed = 0
    for p in spots:
        if placed >= n:
            break
        placed += furnish(m, start, [p], ch)


def scatter(m, start, region, ch, n):
    spots = m.free(region)
    m.rng.shuffle(spots)
    placed = 0
    for p in spots:
        if placed >= n:
            break
        placed += furnish(m, start, [p], ch)


# ----------------------------------------------------------------------------- levels
def tavern():
    m = C.Map(27, 15, seed=1, border="W")
    m.rect(19, 1, 19, 13, "W")
    m.set(19, 7, "D")
    m.rect(2, 3, 9, 3, "b")                          # the bar: a row of shelves with a gap
    m.set(5, 3, ".")
    a = (2, 1)
    m.set(*a, "A")
    m.set(24, 3, "g")
    m.set(2, 12, "k")
    dining(m, a, (3, 6, 17, 12), 7)
    along_walls(m, a, (1, 1, 18, 13), "B", 5)
    along_walls(m, a, (20, 1, 25, 13), "b", 4)
    m.set(22, 11, "d")
    along_walls(m, a, (1, 1, 25, 13), TORCH, 4)
    return m, a, dict(dirt=3, sand=1)


def wine_cellar():
    m = C.Map(33, 19, seed=2, border=ROCK)
    for _ in range(10):
        m.blob(m.rng.randint(2, 30), m.rng.choice((1, 17)), m.rng.uniform(1.0, 2.0), ROCK)
    m.rect(11, 1, 11, 17, "W")
    m.rect(22, 1, 22, 17, "W")
    m.set(11, 13, "D")
    m.set(22, 4, "D")
    a = (2, 9)
    m.set(*a, "A")
    m.set(8, 2, "k")
    m.set(19, 16, "k")
    m.set(29, 9, "g")
    for x0 in (13, 16, 19):                          # wine racks: columns of barrels with gaps
        for y in range(3, 16):
            if y not in (6, 11) and m.get(x0, y) == ".":
                furnish(m, a, [(x0, y)], "B")
    scatter(m, a, (1, 1, 10, 17), "B", 8)
    along_walls(m, a, (23, 1, 31, 17), "b", 5)
    dining(m, a, (24, 8, 30, 15), 2)
    along_walls(m, a, (1, 1, 31, 17), TORCH, 6)
    return m, a, dict(dirt=8, moss=4, bones=3, mush=3)


def inn_upstairs():
    m = C.Map(35, 21, seed=3, border="W")
    m.rect(1, 8, 33, 8, "W")
    m.rect(1, 12, 33, 12, "W")
    for x in (9, 17, 25):
        m.rect(x, 1, x, 7, "W")
        m.rect(x, 13, x, 19, "W")
    # rooms: top (1..8, 10..16, 18..24, 26..33), bottom likewise; door / doorway into the corridor
    tops = [(5, 8, "."), (13, 8, "D"), (21, 8, "."), (29, 8, "D")]
    bots = [(5, 12, "D"), (13, 12, "."), (21, 12, "D"), (29, 12, ".")]
    for x, y, c in tops + bots:
        m.set(x, y, c)
    a = (2, 10)
    m.set(*a, "A")
    m.set(2, 2, "k")                                 # open room
    m.set(32, 18, "k")                               # open room
    m.set(14, 2, "k")                                # behind a door
    m.set(3, 17, "k")                                # behind a door (a spare)
    m.set(32, 2, "g")                                # the dwarf's room
    for (x0, x1) in ((1, 8), (10, 16), (18, 24), (26, 33)):
        for (y0, y1) in ((1, 7), (13, 19)):
            along_walls(m, a, (x0, y0, x1, y1), "b", 2)
            scatter(m, a, (x0 + 1, y0 + 1, x1 - 1, y1 - 1), m.rng.choice("tB"), 2)
            if m.rng.random() < 0.6:
                scatter(m, a, (x0, y0, x1, y1), "c", 1)
    scatter(m, a, (3, 9, 33, 11), TORCH, 0)
    m.set(22, 16, "d")
    return m, a, dict(dirt=4, sand=6)


def brewery():
    m = C.Map(39, 23, seed=4, border="W")
    m.rect(1, 11, 37, 11, "W")                       # upper / lower halls
    m.rect(13, 1, 13, 10, "W")
    m.rect(26, 12, 26, 21, "W")
    m.set(13, 5, "D")
    m.set(30, 11, "D")
    m.set(8, 11, ".")                                # open stairwell between halls
    m.set(9, 11, ".")
    m.set(26, 17, "D")
    # brewing vats (water) crossed by a plank bridge in the upper east hall
    m.rect(15, 4, 35, 6, WATER)
    m.set(20, 4, BRIDGE), m.set(20, 5, BRIDGE), m.set(20, 6, BRIDGE)
    m.set(31, 4, BRIDGE), m.set(31, 5, BRIDGE), m.set(31, 6, BRIDGE)
    a = (2, 2)
    m.set(*a, "A")
    m.set(11, 20, "k")                               # lower west hall (open)
    m.set(35, 2, "k")                                # upper east (behind door 1)
    m.set(24, 14, "k")                               # lower middle (open)
    m.set(35, 19, "g")                               # lower east, behind the third door
    dining(m, a, (2, 13, 24, 20), 6)
    for y in (2, 8):
        for x in range(15, 36, 2):
            if m.rng.random() < 0.6:
                furnish(m, a, [(x, y)], "B")
    along_walls(m, a, (1, 1, 12, 10), "b", 6)
    along_walls(m, a, (27, 12, 37, 21), "b", 3)
    m.set(33, 15, "d")
    along_walls(m, a, (1, 1, 37, 21), TORCH, 6)
    return m, a, dict(dirt=6, sand=3, moss=2)


def dwarven_hall():
    m = C.Map(41, 25, seed=5, fill=STONE, border=STONE)
    halls = [(1, 1, 12, 9), (16, 1, 25, 9), (29, 1, 39, 11), (1, 13, 12, 23), (16, 13, 25, 23), (29, 15, 39, 23)]
    for h in halls:
        m.rect(*h, ".")
    # passages between halls; some are locked doors
    m.rect(13, 5, 15, 5, "."), m.set(14, 5, "D")      # 0 -> 1 locked
    m.rect(26, 4, 28, 4, ".")                         # 1 -> 2 open
    m.rect(6, 10, 6, 12, "."), m.set(6, 11, "D")      # 0 -> 3 locked
    m.rect(13, 18, 15, 18, ".")                       # 3 -> 4 open
    m.rect(34, 12, 34, 14, "."), m.set(34, 13, "D")   # 2 -> 5 locked
    m.rect(26, 20, 28, 20, "."), m.set(27, 20, "D")   # 4 -> 5 locked
    a = (2, 2)
    m.set(*a, "A")
    m.set(11, 8, "k")                                 # hall 0
    m.set(2, 22, "k")                                 # hall 3 (behind a door)
    m.set(37, 2, "k")                                 # hall 2 (behind a door)
    m.set(20, 22, "k")                                # hall 4
    m.set(38, 22, "g")                                # the royal coffin, hall 5
    for i, h in enumerate(halls):
        x0, y0, x1, y1 = h
        for x in range(x0 + 2, x1 - 1, 3):           # pillars
            for y in (y0 + 2, y1 - 2):
                furnish(m, a, [(x, y)], STONE)
        along_walls(m, a, h, TORCH, 3)
        if i in (1, 4):
            dining(m, a, h, 3)
        else:
            scatter(m, a, h, m.rng.choice("Bb"), 4)
    m.blob(20, 5, 1.6, WATER, only=".")
    m.set(8, 16, "d")
    return m, a, dict(dirt=6, moss=5, bones=4, skull=3)


LEVELS = [
    ("The Tavern", "Last orders! Find the key and stagger to bed.", tavern),
    ("Wine Cellar", "Two doors, two keys, a lot of barrels.", wine_cellar),
    ("Inn Upstairs", "Which room was yours again? One key is locked in a guest room.", inn_upstairs),
    ("Brewery", "Cross the vats on the planks. Three doors to the bedchamber.", brewery),
    ("Dwarven Hall", "Six great halls, four locked doors, and a royal coffin.", dwarven_hall),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one dwarf"
    goal = C.find(rows, "g")
    assert len(goal) == 1, f"{name}: need exactly one coffin"
    keys, doors = C.find(rows, "k"), C.find(rows, "D")
    assert len(keys) >= len(doors), f"{name}: fewer keys than doors"
    ki = {p: i for i, p in enumerate(keys)}
    di = {p: i for i, p in enumerate(doors)}

    @lru_cache(maxsize=None)
    def region(opened):
        return frozenset(C.reach(rows, a[0], lambda c, p: c in FLOOR or c in "k" or (c == "D" and opened >> di[p] & 1)))

    def touches(reg, p):
        return any((p[0] + dx, p[1] + dy) in reg for dx, dy in DIRS)

    def succ(state):
        got, opened = state
        reg = region(opened)
        held = bin(got).count("1") - bin(opened).count("1")
        out = []
        for p, i in ki.items():
            if not got >> i & 1 and p in reg:
                out.append((got | 1 << i, opened))
        if held > 0:
            for p, i in di.items():
                if not opened >> i & 1 and touches(reg, p):
                    out.append((got, opened | 1 << i))
        return out

    def won(state):
        return touches(region(state[1]), goal[0])

    start = (0, 0)
    seen, stack = {start}, [start]
    while stack:
        for n in succ(stack.pop()):
            if n not in seen:
                seen.add(n)
                stack.append(n)
    good = {s for s in seen if won(s)}
    assert good, f"{name}: coffin unreachable"
    changed = True
    while changed:  # backwards: states that can reach a winning state
        changed = False
        for s in seen - good:
            if any(n in good for n in succ(s)):
                good.add(s)
                changed = True
    bad = seen - good
    assert not bad, f"{name}: {len(bad)} softlocked states reachable"
    return len(keys), len(doors), len(seen)


# ----------------------------------------------------------------------------- GDY
SOLID = "[wall, chair, table, bookshelf, barrel, doggo, water, tree, bush, rock, hedge, stone, tombstone, torch]"
ACTIONS = f'''
Actions:
{C.face_action("drunk_dwarf")}
  # The dwarf stumbles up / down / left / right (absolute). Keys are picked up by walking into them;
  # walking into a locked door while holding a key uses the key up and unlocks the door.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: drunk_dwarf, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND[:-1]}, open_door]}}
      - Src: {{Object: drunk_dwarf, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}]}}
        Dst: {{Object: coffin_bed, Commands: [{{remove: true}}]}}
      - Src: {{Object: drunk_dwarf, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{incr: keys}}, {{reward: 1}}]}}
        Dst: {{Object: key, Commands: [{{remove: true}}]}}
      - Src:
          Object: drunk_dwarf
          Preconditions: [{{gte: [src.keys, 1]}}]
          Commands: [{{set_tile: meta.facing}}, {{decr: keys}}, {{reward: 1}}]
        Dst: {{Object: door, Commands: [{{change_to: open_door}}]}}
      - Src: {{Object: drunk_dwarf, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: {SOLID}}}
'''


def objects():
    avatar = C.facing_sprites("oryx/oryx_fantasy/avatars/dwarf1.png", "drunkdwarf-dwarf")
    dwarf_extra = "    Variables:\n      - Name: keys\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "W", 1, C.WALL_STYLES["brick"], "[0.7, 0.7, 0.7]", "square", 1.0),
        C.obj("door", "D", 1, C.img("oryx/oryx_fantasy/door-1.png"), "[0.0, 0.0, 0.8]", "square", 1.0),
        C.obj("open_door", None, 1, C.img("oryx/oryx_fantasy/open_door-1.png"), "[0.1, 0.1, 0.1]", "square", 0.3),
        C.obj("key", "k", 1, C.img("oryx/oryx_fantasy/key-3.png"), "[1.0, 1.0, 0.0]", "triangle", 0.5),
        C.obj("coffin_bed", "g", 1, C.img("oryx/oryx_fantasy/coffin-1.png"), "[0.0, 1.0, 0.0]", "square", 0.8),
        C.obj("chair", "c", 1, C.img("oryx/oryx_fantasy/chair-1.png"), "[0.4, 0.0, 0.4]", "triangle", 0.6),
        C.obj("table", "t", 1, C.img("oryx/oryx_fantasy/table-1.png"), "[0.4, 0.4, 0.4]", "square", 0.8),
        C.obj("bookshelf", "b", 1, C.img("oryx/oryx_fantasy/bookshelf-1.png"), "[0.0, 0.4, 0.4]", "square", 0.8),
        C.obj("barrel", "B", 1, C.img("gvgai/oryx/barrel1.png"), "[0.5, 0.3, 0.1]", "circle", 0.7),
        C.obj("doggo", "d", 1, C.img("oryx/oryx_fantasy/avatars/doggo1.png"), "[0.2, 0.2, 0.2]", "triangle", 0.7),
        C.obj("drunk_dwarf", "A", 2, C.imgs(avatar), "[1.0, 0.0, 0.0]", "triangle", 0.9, dwarf_extra, tiles=4),
    ])


def main():
    parts = [C.header("Drunk Dwarf Remastered", """
        You've had a few, and your bed (a coffin?? weird) is behind a locked door... or three.
        Pick up keys by walking into them; walk into a locked door to use up a key and unlock it.
        Tables, chairs, shelves, barrels, vats and the snoozing doggo are in the way.
        """.replace("        ", ""), "drunk_dwarf", ["eq: [coffin_bed:count, 0]"], [],
        background="oryx/oryx_fantasy/floor1-1.png")]
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, _, recipe = build()
        rows = m.rows(solid=WALLS)
        k, d, states = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=9000 + i, stack_under="Akg")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:14s} {len(rows[0])}x{len(rows)} keys={k} doors={d} states={states} (no softlocks)",
              file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    C.emit(GID, gdy, {
        "name": "Drunk Dwarf Remastered",
        "tagline": "Keys, doors and too much ale between you and your bed.",
        "description": "Stagger through taverns, cellars, inns and dwarven halls. Each key unlocks one "
                       "door and is used up; chain the doors to reach your coffin bed.",
        "tags": ["exploration", "puzzle", "classic-port"],
        "difficulty": "medium",
        "original": "Mini-Grid/minigrid-drunkdwarf.yaml",
        "accent": "#f59e0b",
        "icon": "🍺",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Keys held", "kind": "var", "object": "drunk_dwarf", "variable": "keys", "tone": "amber"},
                {"label": "Keys left", "kind": "remaining", "object": "key", "tone": "amber"},
                {"label": "Locked doors", "kind": "remaining", "object": "door", "tone": "rose"}],
        "win_text": "Zzz... safely tucked into your coffin.",
        "lose_text": "Passed out on the floor.",
    })


if __name__ == "__main__":
    main()
