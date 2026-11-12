#!/usr/bin/env python3
"""Labyrinth Remastered: find the way out of big braided mazes without stepping on a spike trap.

Remaster of resources/games/GVGAI/labyrinth.yaml. Same core rules: reach the exit to win, stepping
onto a trap kills you. Levels are 31x21 .. 41x25 braided mazes (common.Map.maze) in different themes
(hedge maze, catacombs, crystal cave, sunken ruins, the deep labyrinth) with open chambers, water,
props tucked into dead ends, and traps placed in corridors. Twist: optional gold coins (+1 each) lie
in dead ends; the level ends as soon as you reach the exit.

Layout characters (besides the shared terrain chars from common):
  w wall (crystal)   A explorer   x exit   t trap   o coin   . floor
Validation: rectangular, one explorer, one exit, the exit and every coin reachable WITHOUT touching a
trap, no trap within 3 steps of the start, and props only placed in dead ends (never cut a path).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

GID = "labyrinth"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TOMB, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                                      "tombstone", "torch"))
BRIDGE = G["bridge"]
SAFE = set(".Axo") | set(G.values())
DIRS = ((1, 0), (-1, 0), (0, 1), (0, -1))


# ----------------------------------------------------------------------------- helpers
def grid(m):
    return ["".join(r) for r in m.g]


def safe_reach(m, start):
    return C.reach(grid(m), start, lambda c, p: c in SAFE)


def open_nbrs(m, x, y):
    return [(x + dx, y + dy) for dx, dy in DIRS if m.get(x + dx, y + dy) in SAFE]


def dead_ends(m):
    return [(x, y) for x, y in m.free() if len(open_nbrs(m, x, y)) == 1]


def traps(m, start, goals, n, min_start=4):
    """Put n traps on straight corridor cells, keeping every goal reachable without touching a trap."""
    spots = [(x, y) for x, y in m.free() if abs(x - start[0]) + abs(y - start[1]) >= min_start
             and len(open_nbrs(m, x, y)) == 2
             and (m.get(x - 1, y) in SAFE and m.get(x + 1, y) in SAFE or m.get(x, y - 1) in SAFE and m.get(x, y + 1) in SAFE)]
    m.rng.shuffle(spots)
    placed = 0
    for p in spots:
        if placed >= n:
            break
        if any(abs(p[0] - q[0]) + abs(p[1] - q[1]) < 3 for q in C.find(grid(m), "t")):
            continue  # spread them out
        m.set(*p, "t")
        r = safe_reach(m, start)
        if all(g in r for g in goals):
            placed += 1
        else:
            m.set(*p, ".")
    return placed


def safe_set(m, start, p, ch):
    """Set a solid char at p unless it would cut off any reachable floor."""
    if m.get(*p) not in SAFE - {"A", "x", "o"}:
        return False
    before = safe_reach(m, start)
    old = m.get(*p)
    m.set(*p, ch)
    if len(safe_reach(m, start)) != len(before) - (p in before):
        m.set(*p, old)
        return False
    return True


def props(m, start, chars, n):
    """Decorate dead ends (never on a needed path)."""
    ends = [p for p in dead_ends(m) if p != start and abs(p[0] - start[0]) + abs(p[1] - start[1]) > 2]
    m.rng.shuffle(ends)
    placed = 0
    for p in ends:
        if placed >= n:
            break
        placed += safe_set(m, start, p, m.rng.choice(chars))


def coins(m, start, n):
    ends = sorted(dead_ends(m), key=lambda p: -abs(p[0] - start[0]) - abs(p[1] - start[1]))
    r = safe_reach(m, start)
    ends = [p for p in ends if p != start and p in r and m.get(*p) == "."]
    m.rng.shuffle(ends)
    out = ends[:n]
    for p in out:
        m.set(*p, "o")
    return out


def chamber(m, x0, y0, x1, y1, fill="."):
    m.rect(x0, y0, x1, y1, fill)


def build(m, start, exit_, n_coins, n_traps, prop_chars, n_props, deco=()):
    m.set(*start, "A")
    for p, ch in deco:  # hand-placed solid decoration, only where it cuts nothing off
        safe_set(m, start, p, ch)
    m.set(*exit_, "x")
    cs = coins(m, start, n_coins)
    props(m, start, prop_chars, n_props)
    traps(m, start, [exit_] + cs, n_traps)
    return m


# ----------------------------------------------------------------------------- levels
def hedge_maze():
    m = C.Map(31, 21, seed=10, border=HEDGE)
    m.maze(0, 0, 30, 20, HEDGE, braid=0.6)
    chamber(m, 13, 9, 17, 11)                        # a little garden in the middle
    deco = [((15, 10), WATER)] + [(p, BUSH) for p in [(13, 9), (17, 9), (13, 11), (17, 11)]]
    build(m, (1, 1), (29, 19), 3, 8, TREE + BUSH, 10, deco)
    return m, dict(grass=16, dirt=3, mush=3)


def catacombs():
    m = C.Map(35, 23, seed=20, border=STONE)
    m.maze(0, 0, 34, 22, STONE, braid=0.5)
    for cx, cy in [(9, 5), (25, 17)]:               # burial chambers
        chamber(m, cx - 2, cy - 2, cx + 2, cy + 2)
    build(m, (1, 21), (33, 1), 4, 12, TOMB + TORCH, 14, [((9, 5), TOMB), ((25, 17), TOMB)])
    return m, dict(dirt=10, bones=16, skull=10, moss=3)


def crystal_cave():
    m = C.Map(37, 23, seed=30, border="w")
    m.maze(0, 0, 36, 22, "w", braid=0.7)
    # erode some walls for a more cavernous feel (only walls between two open cells)
    for y in range(1, 22):
        for x in range(1, 36):
            if m.get(x, y) == "w" and m.rng.random() < 0.08:
                if (m.get(x - 1, y) == "." and m.get(x + 1, y) == ".") or (m.get(x, y - 1) == "." and m.get(x, y + 1) == "."):
                    m.set(x, y, ".")
    m.blob(18, 11, 2.5, WATER)                       # an underground lake...
    m.rect(14, 11, 22, 11, BRIDGE)                   # ...with a plank bridge across
    m.set(13, 11, "."), m.set(23, 11, ".")
    build(m, (1, 1), (35, 21), 4, 12, ROCK + TORCH, 10)
    return m, dict(moss=10, sand=4, mush=8)


def sunken_ruins():
    m = C.Map(39, 25, seed=40, border=ROCK)
    m.maze(0, 0, 38, 24, STONE, braid=0.6)
    for _ in range(7):                               # flooded patches swallow parts of the maze
        cx, cy = m.rng.randint(5, 33), m.rng.randint(4, 20)
        m.blob(cx, cy, m.rng.uniform(1.2, 2.2), WATER, only=STONE)
    # rebuild reachability guarantee: the maze floor is untouched (water only replaced walls)
    build(m, (1, 23), (37, 1), 4, 14, TREE + BUSH + TOMB, 12)
    return m, dict(moss=12, grass=8, sand=4, mush=5)


def deep_labyrinth():
    m = C.Map(41, 25, seed=50, border="w")
    m.maze(0, 0, 40, 24, "w", braid=0.45)
    chamber(m, 17, 9, 23, 15)                        # the minotaur's hall (empty now... probably)
    deco = [(p, TORCH) for p in [(17, 9), (23, 9), (17, 15), (23, 15)]] + [((20, 12), "w")]
    build(m, (20, 13), (1, 1), 5, 18, ROCK + TOMB + TORCH, 14, deco)
    return m, dict(dirt=10, bones=12, skull=8, sand=4)


LEVELS = [
    ("Hedge Maze", "A garden maze. Mind the spikes hidden in the hedges.", hedge_maze),
    ("Catacombs", "Tombs, torches and a lot of spikes. Find the way up.", catacombs),
    ("Crystal Cave", "A cavernous maze around an underground lake.", crystal_cave),
    ("Sunken Ruins", "Half-flooded ruins. Watch your step.", sunken_ruins),
    ("The Deep Labyrinth", "Start in the heart of the labyrinth. The exit is far away.", deep_labyrinth),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one explorer"
    ex = C.find(rows, "x")
    assert len(ex) == 1, f"{name}: need exactly one exit"
    cs = C.find(rows, "o")
    ts = C.find(rows, "t")
    seen = C.reach(rows, a[0], lambda c, p: c in SAFE)
    missing = [p for p in ex + cs if p not in seen]
    assert not missing, f"{name}: unreachable (trap-free) exit/coins {missing}"
    near = [p for p in ts if abs(p[0] - a[0][0]) + abs(p[1] - a[0][1]) < 4]
    assert not near, f"{name}: trap too close to start {near}"
    # shortest trap-free path length to the exit (for the report)
    import collections
    dist, q = {a[0]: 0}, collections.deque([a[0]])
    while q:
        x, y = q.popleft()
        for dx, dy in DIRS:
            n = (x + dx, y + dy)
            if n not in dist and rows[n[1]][n[0]] in SAFE:
                dist[n] = dist[(x, y)] + 1
                q.append(n)
    return len(ts), len(cs), dist[ex[0]]


# ----------------------------------------------------------------------------- GDY
ACTIONS = f'''
Actions:
{C.face_action("avatar")}
  # The explorer walks up / down / left / right. Traps kill, coins score, the exit wins.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: trap}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{reward: 1}}]}}
        Dst: {{Object: coin, Commands: [{{remove: true}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{reward: 1}}]}}
        Dst: {{Object: exit, Commands: [{{remove: true}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, water, tree, bush, rock, hedge, stone, tombstone, torch]}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/newset/girl1.png", "labyrinth-avatar")
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "w", 1, C.WALL_STYLES["teal"], "[0.5, 0.5, 0.5]", "square", 1.0),
        C.obj("exit", "x", 1, C.img("gvgai/newset/exit2.png"), "[0.2, 0.2, 0.8]", "square", 0.8),
        C.obj("trap", "t", 1, C.img("gvgai/oryx/spike2.png"), "[1.0, 0.0, 0.0]", "triangle", 0.4),
        C.obj("coin", "o", 1, C.img("gvgai/oryx/gold1.png"), "[1.0, 0.85, 0.1]", "circle", 0.4),
        C.obj("avatar", "A", 2, C.imgs(avatar), "[0.2, 0.8, 0.2]", "triangle", 0.7, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Labyrinth Remastered", """
        It's a maze: find your way out. Big braided labyrinths in hedges, catacombs, crystal caves and
        sunken ruins. Stepping on a spike trap is fatal. Gold coins in dead ends are a bonus.
        """.replace("        ", ""), "avatar", ["eq: [exit:count, 0]"], ["eq: [avatar:count, 0]"],
        background="gvgai/oryx/backBiege.png")]
    for i, (name, blurb, build_fn) in enumerate(LEVELS):
        m, recipe = build_fn()
        rows = m.rows(solid="w" + HEDGE + STONE + ROCK)
        nt, nc, d = validate(name, rows)
        ground = C.decorate(rows, recipe, seed=6000 + i, stack_under="Axo")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:18s} {len(rows[0])}x{len(rows)} traps={nt} coins={nc} shortest_exit={d}", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    C.emit(GID, gdy, {
        "name": "Labyrinth Remastered",
        "tagline": "Find the way out. Don't step on the spikes.",
        "description": "Navigate huge braided mazes (hedges, catacombs, crystal caves, sunken ruins) to "
                       "the exit. Spike traps lurk in the corridors; gold in the dead ends is a bonus.",
        "tags": ["maze", "exploration", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/labyrinth.yaml",
        "accent": "#2dd4bf",
        "icon": "🌀",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Gold", "kind": "collected", "object": "coin", "tone": "amber"}],
        "win_text": "You escaped the labyrinth!",
        "lose_text": "Impaled on a spike trap.",
    })


if __name__ == "__main__":
    main()
