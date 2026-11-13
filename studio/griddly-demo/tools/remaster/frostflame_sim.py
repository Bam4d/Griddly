"""Frost & Flame rules simulator + solver. Mirrors the GDY written by frostflame.py move for move
(frostflame.py --diff replays random walks in the WASM engine and compares every state).

Layers (as in the engine):
  Z0 floor   plain snow / ground decoration, `ice` (slippery), `pond` (open water: blocks walking)
  Z1 things  walls + scenery, snowman, frost wall, socket (unlit beacon), beacon (lit), shrine,
             cube (ice block), brazier, avatar

State = (player, cubes, braziers, ice, pond, melt, sockets, beacons, shrine) - all frozensets of
cells except the player. `melt` = snowmen + frost walls (both melt; never move).

One player move in direction d:
  * onto floor: walk. Onto ice: slide on until the next cell is not ice (step onto it if it is free
    floor) or blocked (any Z1 thing or a pond: stop before it). Walking/sliding into the shrine takes it.
  * into a cube/brazier: push it.
      cube    -> floor: moves; -> ice: slides like the avatar; -> pond: sinks and freezes it into ice
      brazier -> floor: moves; -> ice: thaws that ice cell into pond and goes out (fire + ice = water);
                 -> pond: goes out; -> socket: lights the beacon (brazier gone, socket -> beacon)
    If it left its cell, the avatar steps into that cell and, if the cell is ice, slides on.
    (Braziers therefore never stand on ice; cubes stand on ice exactly when their `slip` var is 1.)
  * end of the move: every fire (brazier, lit beacon) heats its 8 neighbours: snowmen, frost walls
    and ice blocks there melt away. Ice floor is not affected by heat, only by contact.
Win: no sockets and no shrine left. Lose: fewer braziers than unlit sockets.

`Rules` is the readable reference implementation on sets of cells; `Fast` is the same rules on bitmask
states with a BFS over push/slide events (used by frostflame.py; cross-checked against `Rules`).
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pushlib as P  # noqa: E402

DIRS = P.DIRS
NB8 = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (dx, dy) != (0, 0)]
FIELDS = ("player", "cubes", "braz", "ice", "pond", "melt", "sockets", "beacons", "shrine")


def add(p, d):
    return p[0] + d[0], p[1] + d[1]


class Rules:
    """`flags` disables mechanics for ablation checks: 'slide' (ice is plain floor: nothing slides), 'noice' (nothing can enter ice), 'freeze' (cubes
    can't enter ponds), 'heat' (fires melt nothing), 'push_cube', 'push_brazier'."""

    def __init__(self, walls, cells, flags=()):
        self.walls = frozenset(walls)
        self.cells = frozenset(cells)
        self.flags = set(flags)
        self.adj = P.make_adj(self.cells)  # pushlib.region fast path (static cells minus blockers())

    def blockers(self, s):
        b = s[1] | s[4] | s[5] | s[6] | s[7] | s[8] | s[2]
        return b if "slide" in self.flags else b | s[3]

    def events(self, s, par):
        """Moves from the avatar's walkable region that change more than the avatar's cell: into ice
        (slide), a cube/brazier (push) or the shrine."""
        hot = s[1] | s[2] | s[8] | (frozenset() if "slide" in self.flags else s[3])
        out = []
        for c in par:
            for m, n in self.adj[c]:
                if n in hot:
                    out.append((c, m))
        return out

    def thing(self, s, c):
        if c in self.walls or c not in self.cells:
            return "wall"
        for name, i in (("cube", 1), ("brazier", 2), ("melt", 5), ("socket", 6), ("beacon", 7), ("shrine", 8)):
            if c in s[i]:
                return name
        return None

    def passable(self, s, n):
        return self.thing(s, n) is None and n not in s[4] and (n not in s[3] or "slide" in self.flags)

    def done(self, s):
        return not s[6] and not s[8]

    def lost(self, s):
        return len(s[2]) < len(s[6])

    def h(self, s):
        return 0

    def move(self, s, m):
        d = DIRS[m]
        player = s[0]
        cubes, braz, ice, pond, melt, sockets, beacons, shrine = (set(x) for x in s[1:])
        slide = "slide" not in self.flags
        noice = "noice" in self.flags
        st = (None, cubes, braz, ice, pond, melt, sockets, beacons, shrine)
        thing = lambda c: self.thing(st, c)  # noqa: E731

        def push(kind, pos):
            if ("push_" + kind) in self.flags:
                return False
            own = cubes if kind == "cube" else braz
            cur = pos
            while True:
                t = add(cur, d)
                th = thing(t)
                if kind == "brazier" and th == "socket":
                    own.discard(cur)
                    sockets.discard(t)
                    beacons.add(t)
                    return True
                if th is not None or (noice and t in ice):
                    break
                if kind == "brazier" and t in ice and slide:
                    ice.discard(t)
                    pond.add(t)
                    own.discard(cur)
                    return True
                if t in pond:
                    if kind == "cube":
                        if "freeze" in self.flags:
                            break
                        pond.discard(t)
                        ice.add(t)
                    own.discard(cur)
                    return True
                own.discard(cur)
                own.add(t)
                cur = t
                if not (slide and t in ice):
                    break
            return cur != pos

        def slide_from(p):
            while True:
                t = add(p, d)
                th = thing(t)
                if th == "shrine":
                    shrine.discard(t)
                    return t
                if th is not None or t in pond:
                    return p
                p = t
                if not (slide and t in ice):
                    return p

        n = add(player, d)
        th = thing(n)
        if th == "shrine":
            shrine.discard(n)
            player = n
        elif th in ("cube", "brazier"):
            if push(th, n):
                player = n
                if slide and n in ice:
                    player = slide_from(n)
        elif th is None and n not in pond and not (noice and n in ice):
            player = n
            if slide and n in ice:
                player = slide_from(n)
        if "heat" not in self.flags:
            hot = {add(f, dd) for f in list(braz) + list(beacons) for dd in NB8}
            for c in hot:
                melt.discard(c)
                cubes.discard(c)
        return (player,) + tuple(frozenset(x) for x in (cubes, braz, ice, pond, melt, sockets, beacons, shrine))

    def step(self, s, cell, m):
        s = (cell,) + tuple(s[1:])
        ns = self.move(s, m)
        if ns == s or self.lost(ns):
            return None
        return ns, self.done(ns)


def parse(rows, wall_chars):
    """Chars: A avatar, i ice, p pond, b cube, f brazier, s snowman, z frost wall, x socket, g shrine.
    `wall_chars` are static blockers; anything else is floor."""
    walls, cells = set(), set()
    sets = {k: set() for k in FIELDS[1:]}
    player = None
    key = {"i": "ice", "p": "pond", "b": "cubes", "f": "braz", "s": "melt", "z": "melt", "x": "sockets",
           "g": "shrine"}
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            p = (x, y)
            cells.add(p)
            if c in wall_chars:
                walls.add(p)
            elif c == "A":
                player = p
            elif c in key:
                sets[key[c]].add(p)
    s = (player,) + tuple(frozenset(sets[k]) for k in FIELDS[1:])
    return walls, cells - walls, s


def solve(rows, wall_chars, flags=(), max_nodes=400_000):
    """Reference solver (pushlib over Rules; slow). Prefer Fast(rows, wall_chars, flags).solve()."""
    walls, cells, s = parse(rows, wall_chars)
    rules = Rules(walls, cells, flags)
    return P.solve(s, rules, max_nodes=max_nodes, weight=0.0), rules, s


def replay(rules, s, moves):
    """Returns (won exactly at the last move, final state)."""
    for i, m in enumerate(moves):
        s = rules.move(s, m)
        if rules.done(s):
            return i == len(moves) - 1, s
    return False, s


def render(rows, s, wall_chars):
    g = [[c if c in wall_chars else "." for c in r] for r in rows]
    for ch, i in (("i", 3), ("p", 4), ("b", 1), ("f", 2), ("s", 5), ("x", 6), ("X", 7), ("g", 8)):
        for x, y in s[i]:
            g[y][x] = ch
    x, y = s[0]
    g[y][x] = "A"
    return "\n".join("".join(r) for r in g)


# ----------------------------------------------------------------------------------------------
# Fast solver: the same rules on bitmask states (cell index y*W+x; every set is a Python int).
# The walkable region of the avatar is a bit-parallel flood fill; search nodes are keyed by
# (region, rest of state), so walking between events never creates new nodes.
# ----------------------------------------------------------------------------------------------
class Fast:
    def __init__(self, rows, wall_chars, flags=()):
        self.rows = rows
        self.H, self.W = len(rows), len(rows[0])
        walls, cells, s = parse(rows, wall_chars)
        self.flags = set(flags)
        W = self.W
        self.walls = self.mask(walls)
        self.D = {1: -1, 2: -W, 3: 1, 4: W}
        self.nb8 = [0] * (W * self.H)
        for y in range(1, self.H - 1):
            for x in range(1, W - 1):
                m = 0
                for dx, dy in NB8:
                    m |= 1 << ((y + dy) * W + x + dx)
                self.nb8[y * W + x] = m
        self.start = (s[0][1] * W + s[0][0],) + tuple(self.mask(x) for x in s[1:])

    def mask(self, cells):
        m = 0
        for x, y in cells:
            m |= 1 << (y * self.W + x)
        return m

    def cells(self, m):
        out = []
        while m:
            low = m & -m
            i = low.bit_length() - 1
            out.append((i % self.W, i // self.W))
            m ^= low
        return out

    def to_slow(self, s):
        p = s[0]
        return ((p % self.W, p // self.W),) + tuple(frozenset(self.cells(x)) for x in s[1:])

    def solid(self, s):
        return self.walls | s[1] | s[2] | s[5] | s[6] | s[7] | s[8]

    def free(self, s):
        """Cells the avatar can simply walk on."""
        f = ~(self.solid(s) | s[4])
        if "slide" not in self.flags:
            f &= ~s[3]
        return f

    def region(self, s, free=None):
        f = self.free(s) if free is None else free
        W = self.W
        r = 1 << s[0]
        while True:
            n = r | (((r << 1) | (r >> 1) | (r << W) | (r >> W)) & f)
            if n == r:
                return r
            r = n

    def move(self, s, m):
        d = self.D[m]
        p, cubes, braz, ice, pond, melt, sockets, beacons, shrine = s
        fl = self.flags
        slide = "slide" not in fl
        walls = self.walls

        def solid():
            return walls | cubes | braz | melt | sockets | beacons | shrine

        n = p + d
        nb = 1 << n
        if shrine & nb:
            shrine &= ~nb
            p = n
        elif (cubes | braz) & nb:
            kind = "cube" if cubes & nb else "brazier"
            moved = False
            if ("push_" + kind) not in fl:
                cur = n
                while True:
                    t = cur + d
                    tb = 1 << t
                    sol = solid()
                    if kind == "brazier" and sockets & tb:
                        braz &= ~(1 << cur)
                        sockets &= ~tb
                        beacons |= tb
                        moved = True
                        break
                    if sol & tb or ("noice" in fl and ice & tb):
                        break
                    if kind == "brazier" and slide and ice & tb:
                        ice &= ~tb
                        pond |= tb
                        braz &= ~(1 << cur)
                        moved = True
                        break
                    if pond & tb:
                        if kind == "cube":
                            if "freeze" in fl:
                                break
                            pond &= ~tb
                            ice |= tb
                            cubes &= ~(1 << cur)
                        else:
                            braz &= ~(1 << cur)
                        moved = True
                        break
                    if kind == "cube":
                        cubes = (cubes & ~(1 << cur)) | tb
                    else:
                        braz = (braz & ~(1 << cur)) | tb
                    cur = t
                    moved = True
                    if not (slide and ice & tb):
                        break
            if moved:
                p = n
                if slide and ice & nb:
                    p = self._slide(p, d, solid(), pond, ice, shrine)
                    if shrine & (1 << p):
                        shrine &= ~(1 << p)
        elif not (solid() & nb) and not (pond & nb) and not ("noice" in fl and ice & nb):
            p = n
            if slide and ice & nb:
                p = self._slide(p, d, solid(), pond, ice, shrine)
                if shrine & (1 << p):
                    shrine &= ~(1 << p)
        if "heat" not in fl and (braz or beacons):
            hot = 0
            for f in self.cells_idx(braz | beacons):
                hot |= self.nb8[f]
            melt &= ~hot
            cubes &= ~hot
        return (p, cubes, braz, ice, pond, melt, sockets, beacons, shrine)

    @staticmethod
    def _slide(p, d, sol, pond, ice, shrine):
        """Avatar slide from p (on ice); stops before solid things/water, enters the shrine."""
        while True:
            t = p + d
            tb = 1 << t
            if sol & tb:
                return t if shrine & tb else p
            if pond & tb:
                return p
            p = t
            if not (ice & tb):
                return p

    @staticmethod
    def cells_idx(m):
        out = []
        while m:
            low = m & -m
            out.append(low.bit_length() - 1)
            m ^= low
        return out

    def done(self, s):
        return not s[6] and not s[8]

    def lost(self, s):
        return bin(s[2]).count("1") < bin(s[6]).count("1")

    def solve(self, max_nodes=2_000_000):
        """BFS over events. Returns (moves, events, nodes) or None."""
        import collections as _c
        self.capped = False
        s0 = self.start
        W = self.W
        free0 = self.free(s0)
        r0 = self.region(s0, free0)
        k0 = (r0,) + s0[1:]
        parent = {k0: None}
        q = _c.deque([(s0, r0, k0)])
        nodes = 0
        DIRS4 = ((1, -1), (2, -W), (3, 1), (4, W))
        while q:
            s, reg, k = q.popleft()
            nodes += 1
            if nodes > max_nodes:
                self.capped = True
                return None
            hot = s[1] | s[2] | s[8]
            if "slide" not in self.flags:
                hot |= s[3]
            for m, d in DIRS4:
                # source cells c in region with c+d in hot: shift hot by -d
                src = (hot >> d if d > 0 else hot << -d) & reg
                for c in self.cells_idx(src):
                    st = (c,) + s[1:]
                    ns = self.move(st, m)
                    if ns == st or self.lost(ns):
                        continue
                    if self.done(ns):
                        parent_chain = [(k, c, m)]
                        return self._rebuild(parent, parent_chain, s0), len(self._events), nodes
                    nf = self.free(ns)
                    nr = self.region(ns, nf)
                    nk = (nr,) + ns[1:]
                    if nk in parent:
                        continue
                    parent[nk] = (k, c, m, s)
                    q.append((ns, nr, nk))
        return None

    def _rebuild(self, parent, chain, s0):
        # chain: [(key, cell, move)] of the final event; walk parents back to the root
        events = []
        k, c, m = chain[0]
        events.append((k, c, m))
        while parent[k] is not None:
            pk, pc, pm, _ = parent[k]
            events.append((pk, pc, pm))
            k = pk
        events.reverse()
        self._events = events
        # replay forward from s0 reconstructing walking paths
        moves = []
        s = s0
        for _, c, m in events:
            moves += self.path(s, c)
            s = self.move((c,) + s[1:], m)
            moves.append(m)
        return moves

    def path(self, s, goal):
        import collections as _c
        W = self.W
        f = self.free(s)
        start = s[0]
        if start == goal:
            return []
        prev = {start: None}
        q = _c.deque([start])
        while q:
            c = q.popleft()
            for m, d in ((1, -1), (2, -W), (3, 1), (4, W)):
                n = c + d
                if n in prev or not (f >> n) & 1:
                    continue
                prev[n] = (c, m)
                if n == goal:
                    out = []
                    while prev[n] is not None:
                        n, mm = prev[n]
                        out.append(mm)
                    return out[::-1]
                q.append(n)
        raise AssertionError("path: goal not reachable")
