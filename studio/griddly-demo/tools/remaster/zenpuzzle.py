#!/usr/bin/env python3
"""Zen Puzzle Remastered: rake every tile of the stone patterns in a zen garden, never twice.

Remaster of resources/games/Single-Player/GVGAI/zenpuzzle.yaml. Same rules: stepping onto a `ground`
tile turns it into `walked` (+1); walked tiles can't be stepped on again; you win when no ground is
left and lose after 1000 steps. The avatar walks freely over the garden around the patterns (sand,
grass, moss), so a pattern may be covered in several runs, each entering and leaving at its edge.
Patterns here are bigger (50-90 tiles per level), constrained by stone lanterns (rocks) and ponds.

Layout characters (besides the shared terrain chars from common):
  A avatar   g ground (pattern tile)   r zen rock   . garden floor
Every level is validated: rectangular, one avatar, the free garden is connected (so walked tiles can
never cut the avatar off), and a DFS path-cover solver finds a way to walk every tile exactly once
(segments start/end at tiles next to the garden; only the last one may end inside). The solution is
printed and written to tools/remaster/solutions/zenpuzzle.json (move ids for the engine replay).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import pushlib as P  # noqa: E402

GID = "zenpuzzle"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, TREE, BUSH, ROCK, HEDGE, STONE, TORCH = (S[k] for k in ("water", "tree", "bush", "rock", "hedge", "stone",
                                                               "torch"))
SAND, BRIDGE = G["sand"], G["bridge"]
FREE = set(".A") | set(G.values())
DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}

# ----------------------------------------------------------------------------- patterns
# g = tile, r = rock (stone lantern), ~ = pond, space = garden floor (not part of the pattern)
PATTERNS = {
    "slab": """
gggggggg
gggggggg
ggrggrgg
gggggggg
gggrgggg
gggggggg
""",
    "twin_a": """
ggggggg
grgggrg
ggggggg
gggrggg
ggggggg
""",
    "twin_b": """
gggggg
ggrggg
gggggg
gggggr
gggggg
ggrggg
""",
    "pond_ring": """
gggggggggg
ggrggggrgg
gg~~~~~~gg
gg~~~~~~gg
gg~~~~~~gg
ggrggggrgg
gggggggggg
""",
    "cross": """
   gggg   
   grgg   
   gggg   
gggggggggg
grggrggggr
gggggggggg
   gggg   
   ggrg   
   gggg   
""",
    "moss": """
 gggggggggg
grggggggggg
gggggrggggg
ggggggrrggg
grggggggrgg
gggggggggg 
""",
    "spine": """
ggggggggggggg
ggggggggrgggg
grggggggrggrg
ggggggggggggg
ggggggggrgggg
ggrrrggggrggg
ggggggggggggg
""",
    "small_a": """
gggg
grgg
gggg
gggr
""",
    "small_b": """
ggggg
ggrgg
ggggg
""",
}


def pattern(name):
    lines = PATTERNS[name].strip("\n").split("\n")
    w = max(len(line) for line in lines)
    return [line.ljust(w) for line in lines]


def stamp(m, name, x0, y0):
    for dy, line in enumerate(pattern(name)):
        for dx, c in enumerate(line):
            if c != " ":
                m.g[y0 + dy][x0 + dx] = {"g": "g", "r": "r", "~": WATER}[c]


def reserve(m, name, x0, y0):
    """Cells of the pattern's bounding box + 1-cell margin (keep scenery away)."""
    p = pattern(name)
    return {(x, y) for y in range(y0 - 1, y0 + len(p) + 1) for x in range(x0 - 1, x0 + len(p[0]) + 1)}


def garden(w, h, seed, border, stamps, ponds=(), trees=0, bushes=0, torches=0, avatar=(1, 1)):
    m = C.Map(w, h, seed, border=border)
    res = set()
    for name, x, y in stamps:
        res |= reserve(m, name, x, y)
    for cx, cy, r in ponds:
        cells = [(x, y) for y in range(int(cy - r - 1), int(cy + r + 2)) for x in range(int(cx - r - 1), int(cx + r + 2))]
        m.blob(cx, cy, r, WATER, only=".")
        for x, y in cells:  # never let a pond creep into a pattern's margin
            if (x, y) in res and m.inside(x, y) and m.get(x, y) == WATER:
                m.set(x, y, ".")
    for name, x, y in stamps:
        stamp(m, name, x, y)
    m.set(*avatar, "A")
    free_ok = lambda: [(x, y) for x, y in m.free() if (x, y) not in res and (x, y) != avatar]

    def props(ch, n):
        spots = free_ok()
        m.rng.shuffle(spots)
        placed = 0
        for p in spots:
            if placed >= n:
                break
            before = len(open_area(m, avatar))
            m.set(*p, ch)
            if len(open_area(m, avatar)) != before - 1:
                m.set(*p, ".")
            else:
                placed += 1

    props(TREE, trees)
    props(BUSH, bushes)
    props(TORCH, torches)
    return m


def open_area(m, start):
    rows = ["".join(r) for r in m.g]
    return C.reach(rows, start, lambda c, p: c in FREE)


def raked_sand():
    return garden(24, 14, 1, HEDGE, [("slab", 8, 4)], trees=6, bushes=8, torches=2, avatar=(3, 7)), \
        dict(sand=10, grass=3, mush=2)


def twin_stones():
    return garden(28, 15, 2, HEDGE, [("twin_a", 4, 4), ("twin_b", 17, 4)], ponds=[(13, 12, 1.8)], trees=8,
                  bushes=8, torches=3, avatar=(13, 2)), dict(sand=12, grass=4, moss=2)


def koi_pond():
    return garden(28, 17, 3, TREE, [("pond_ring", 9, 5), ("small_b", 21, 2)], ponds=[(3, 13, 1.6)], trees=10,
                  bushes=10, torches=2, avatar=(2, 2)), dict(grass=10, sand=6, moss=4, mush=3)


def lantern_cross():
    return garden(30, 17, 4, HEDGE, [("cross", 3, 4), ("small_a", 21, 3), ("small_b", 20, 11)], ponds=[(26, 9, 1.4)],
                  trees=8, bushes=10, torches=6, avatar=(15, 1)), dict(sand=12, moss=4, grass=4)


def moss_garden():
    return garden(30, 18, 5, TREE, [("moss", 4, 3), ("twin_b", 20, 9), ("small_a", 5, 12)], ponds=[(24, 3, 1.8)],
                  trees=12, bushes=12, torches=2, avatar=(16, 9)), dict(moss=14, grass=8, mush=6)


def dragons_spine():
    return garden(34, 19, 6, STONE, [("spine", 3, 3), ("small_a", 24, 4), ("small_b", 21, 12)], ponds=[(9, 15, 2.2)],
                  trees=10, bushes=12, torches=8, avatar=(17, 16)), dict(sand=16, moss=4, grass=4, skull=2)


LEVELS = [
    ("Raked Sand", "One stone slab, two lanterns. Rake every tile exactly once.", raked_sand),
    ("Twin Stones", "Two patterns side by side, around a little pond.", twin_stones),
    ("Koi Pond", "A ring of tiles around the koi pond — plan your laps.", koi_pond),
    ("Lantern Cross", "A cross of tiles between stone lanterns, plus two small slabs.", lantern_cross),
    ("Moss Garden", "Three mossy patterns studded with rocks.", moss_garden),
    ("Dragon's Spine", "A long spine of tiles. Ninety steps of perfect calm.", dragons_spine),
]


# ----------------------------------------------------------------------------- solver
def solve(rows, max_nodes=2_000_000):
    """Path cover of all `g` cells: segments enter/leave at tiles next to the reachable garden; only the
    very last segment may end inside a pattern. Patterns (4-connected tile groups) are independent, as
    the garden between them is free to walk, so each is solved on its own; at most one may need an
    inside ending and it is walked last. Returns (list of segments (lists of cells), nodes) or None."""
    a = C.find(rows, "A")[0]
    garden_cells = C.reach(rows, a, lambda c, p: c in FREE)
    tiles = set(C.find(rows, "g"))
    comps = []
    while tiles:
        comp = C.reach(rows, next(iter(tiles)), lambda c, p: c == "g")
        comps.append(sorted(comp))
        tiles -= comp
    out, last, total = [], None, 0
    for comp in comps:
        segs, n = solve_cells(comp, garden_cells, True, max_nodes)
        total += n
        if segs is None:
            segs, n = solve_cells(comp, garden_cells, False, max_nodes)
            total += n
            if segs is None or last is not None:
                return None, total
            last = segs
        else:
            out += segs
    return out + (last or []), total


def solve_cells(cells, garden_cells, strict, max_nodes):
    """Path cover of one pattern; strict = every segment must also leave at an edge tile."""
    idx = {p: i for i, p in enumerate(cells)}
    n = len(cells)
    nb = [[idx[(x + dx, y + dy)] for dx, dy in DIRS.values() if (x + dx, y + dy) in idx] for x, y in cells]
    edge = [any((x + dx, y + dy) in garden_cells for dx, dy in DIRS.values()) for x, y in cells]
    edge_mask = sum(1 << i for i in range(n) if edge[i])
    nodes = [0]
    path: list[int] = []

    def feasible(vis, cur):
        # Edge tiles connect to the garden, which can be re-entered any number of times, so they never
        # dead-end. An interior tile with a single way in must be the final tile of the whole walk.
        unv = [i for i in range(n) if not vis >> i & 1]
        ends = 0
        for i in unv:
            if edge[i]:
                continue
            k = sum(1 for j in nb[i] if not vis >> j & 1) + (cur is not None and i in nb[cur])
            if k == 0:
                return False
            if k == 1:
                ends += 1
        if ends > (0 if strict else 1):
            return False
        # components: each must contain an edge tile, or be reachable from cur (then it must be the only one)
        seen = 0
        comps = []
        for i in unv:
            if seen >> i & 1:
                continue
            stack, comp = [i], 1 << i
            seen |= 1 << i
            while stack:
                c = stack.pop()
                for j in nb[c]:
                    if not (vis >> j & 1) and not (comp >> j & 1):
                        comp |= 1 << j
                        seen |= 1 << j
                        stack.append(j)
            comps.append(comp)
        for comp in comps:
            if not comp & edge_mask:
                if cur is None or not any(comp >> j & 1 for j in nb[cur]) or len(comps) > 1:
                    return False
        return True

    def onward(vis, i):
        return sum(1 for j in nb[i] if not vis >> j & 1)

    failed = set()

    def dfs(cur, vis, left):
        if left == 0:
            return not strict or edge[cur]
        key = (cur, vis)
        if key in failed:
            return False
        nodes[0] += 1
        if nodes[0] > max_nodes:
            return False
        if not feasible(vis, cur) or not search(cur, vis, left):
            failed.add(key)
            return False
        return True

    def search(cur, vis, left):
        opts = []
        if cur is not None:
            opts = sorted((j for j in nb[cur] if not vis >> j & 1), key=lambda j: (onward(vis, j), -edge[j]))
        for j in opts:
            path.append(j)
            if dfs(j, vis | 1 << j, left - 1):
                return True
            path.pop()
        if cur is None or edge[cur]:  # leave the pattern (or start) and enter at another edge tile
            ents = sorted((j for j in range(n) if edge[j] and not vis >> j & 1), key=lambda j: onward(vis, j))
            for j in ents:
                path.append(-1)
                path.append(j)
                if dfs(j, vis | 1 << j, left - 1):
                    return True
                path.pop()
                path.pop()
        return False

    ok = dfs(None, 0, n)
    if not ok:
        return None, nodes[0]
    segs, cur = [], None
    for v in path:
        if v == -1:
            cur = []
            segs.append(cur)
        else:
            cur.append(cells[v])
    return segs, nodes[0]


def bfs_path(rows, start, goals, blocked):
    """Moves over garden cells (FREE, minus `blocked`) from start to any goal cell."""
    import collections
    par = {start: None}
    q = collections.deque([start])
    while q:
        c = q.popleft()
        if c in goals:
            out = []
            while par[c] is not None:
                c, mv = par[c]
                out.append(mv)
            return out[::-1], c if not out else None
        for mv, (dx, dy) in DIRS.items():
            nx, ny = c[0] + dx, c[1] + dy
            if (nx, ny) not in par and rows[ny][nx] in FREE and (nx, ny) not in blocked:
                par[(nx, ny)] = (c, mv)
                q.append((nx, ny))
    return None


def moves_for(rows, segs):
    """Turn path segments into engine move ids starting from the avatar."""
    pos = C.find(rows, "A")[0]
    moves = []
    for seg in segs:
        e = seg[0]
        goals = {(e[0] - dx, e[1] - dy) for dx, dy in DIRS.values()}
        goals = {g for g in goals if rows[g[1]][g[0]] in FREE}
        import collections
        par = {pos: None}
        q = collections.deque([pos])
        end = None
        while q:
            c = q.popleft()
            if c in goals:
                end = c
                break
            for mv, (dx, dy) in DIRS.items():
                nx, ny = c[0] + dx, c[1] + dy
                if (nx, ny) not in par and rows[ny][nx] in FREE:
                    par[(nx, ny)] = (c, mv)
                    q.append((nx, ny))
        assert end is not None, f"cannot reach entry {e}"
        walk, c = [], end
        while par[c] is not None:
            c, mv = par[c]
            walk.append(mv)
        moves += walk[::-1]
        cur = end
        for nxt in seg:
            d = (nxt[0] - cur[0], nxt[1] - cur[1])
            moves.append(next(k for k, v in DIRS.items() if v == d))
            cur = nxt
        # step off the pattern back into the garden (unless this is the last segment)
        if seg is not segs[-1]:
            out = next(((cur[0] + dx, cur[1] + dy), k) for k, (dx, dy) in DIRS.items()
                       if rows[cur[1] + dy][cur[0] + dx] in FREE)
            moves.append(out[1])
            cur = out[0]
        pos = cur
    return moves


def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one avatar"
    free = {p for p in ((x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in FREE)}
    seen = C.reach(rows, a[0], lambda c, p: c in FREE)
    assert seen == free, f"{name}: garden not connected ({len(free - seen)} cells cut off)"
    tiles = C.find(rows, "g")
    assert 40 <= len(tiles) <= 140, f"{name}: {len(tiles)} tiles"
    segs, nodes = solve(rows)
    assert segs, f"{name}: no solution found ({nodes} nodes)"
    assert sorted(c for s in segs for c in s) == sorted(tiles)
    return len(tiles), segs, nodes


# ----------------------------------------------------------------------------- GDY
ACTIONS = f'''
Actions:
{C.face_action("monk")}
  # The monk walks freely through the garden. Stepping onto a pattern tile rakes it (walked, +1);
  # raked tiles, rocks and scenery block the way.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: monk, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: monk, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: ground, Commands: [{{change_to: walked}}, {{reward: 1}}]}}
      - Src: {{Object: monk, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [walked, zen_rock, {", ".join(C.SCENERY_NAMES)}]}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/oryx/angel1.png", "zenpuzzle-monk")
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("ground", "g", 1, C.img("gvgai/oryx/floorTileOrange.png"), "[0.2, 0.2, 0.4]", "square", 0.9),
        C.obj("walked", None, 1, C.img("gvgai/oryx/floorTileGreen.png"), "[0.2, 0.6, 0.2]", "square", 0.9),
        C.obj("zen_rock", "r", 1, C.img("gvgai/oryx/wall5.png"), "[0.2, 0.2, 0.2]", "triangle", 0.8),
        C.obj("monk", "A", 2, C.imgs(avatar), "[0.8, 0.2, 0.2]", "triangle", 0.8, C.FACE_INITIAL_ACTION, tiles=4),
    ])


def main():
    parts = [C.header("Zen Puzzle Remastered", """
        Rake every stone tile of the zen garden's patterns, but never step on a tile twice. Walk freely
        over the sand, grass and moss around the patterns; rocks, lanterns, ponds, trees and bushes block
        the way. You win when every tile is raked; you lose after 1000 steps.
        """.replace("        ", ""), "monk", ["eq: [ground:count, 0]"], ["eq: [_steps, 1000]"],
        background="oryx/oryx_fantasy/floor3-0.png")]
    sols = []
    for i, (name, blurb, build) in enumerate(LEVELS):
        m, recipe = build()
        rows = m.rows(solid=HEDGE + TREE + STONE)
        n, segs, nodes = validate(name, rows)
        moves = moves_for(rows, segs)
        sols.append(moves)
        ground = C.decorate(rows, recipe, seed=4200 + i, floor=".", stack_under="A")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:15s} {len(rows[0])}x{len(rows)} tiles={n} solution: {len(segs)} run(s), "
              f"{len(moves)} moves, {nodes} search nodes", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
            print("  runs:", [[s[0], s[-1], len(s)] for s in segs], file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    P.write_solutions(GID, sols)  # tools/remaster/solutions/<id>.json, read by replay_solutions.mjs
    C.emit(GID, gdy, {
        "name": "Zen Puzzle Remastered",
        "tagline": "Rake every tile of the garden — exactly once.",
        "description": "Walk the zen garden and step on every stone tile of its patterns exactly once. "
                       "Lanterns and ponds break up the patterns; you may leave a pattern at its edge and "
                       "come back in elsewhere, but every raked tile is gone for good.",
        "tags": ["puzzle", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/zenpuzzle.yaml",
        "accent": "#f59e0b",
        "icon": "🪨",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Tiles left", "kind": "remaining", "object": "ground", "tone": "amber"},
                {"label": "Raked", "kind": "count", "object": "walked", "tone": "emerald"}],
        "win_text": "Perfect calm. Every tile raked exactly once.",
        "lose_text": "The garden's patience ran out.",
    })


if __name__ == "__main__":
    main()
