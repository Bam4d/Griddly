"""Reference simulator + planner for Timber & Tide (lumberjack.py).

The simulator mirrors the GDY rules exactly (checked against the WASM engine with
`lumberjack.py --diff`, which replays random action sequences in both and compares every state):

  * a step = the player's action, then tick += 1, then every delayed event whose time <= tick runs
    (the engine's priority queue). Fire is fully deterministic: no Randomize, no Probability, no
    Triggers, so a recorded run replays exactly.
  * fire objects (fire, bridgefire, campfire) carry an entity id; events of an object that has since
    been removed (doused, burnt out) are dropped, like the engine drops delayed actions whose source
    object no longer exists.

The planner (`solve`) is a best-first search over *macro* actions on top of the exact simulator:
walk (BFS path on the current map) to a cell next to a target and act on it (chop a tree, bridge a
water cell, drop a log pile as a fuse, fill / pour the bucket, pick up a hen), or wait a few ticks.
Macro targets are pruned with a relaxed route (Dijkstra where water / rapids / trees / brambles /
fires cost extra) from the lumberjack to the remaining hens. States are de-duplicated on everything
that matters for the future (position, logs, bucket, map, pending fire events relative to now).
"""
from __future__ import annotations

import heapq
import itertools

S = 3   # a fire tries to spread to each neighbour every S ticks
B = 7   # a (non-camp) fire burns out B ticks after it was lit
CAP = 3  # max logs carried

DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
GROUND = {"grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge", "stump"}
WALKABLE = GROUND | {None}
FLAMMABLE = {"tree", "oak", "oakcut", "bramble", "logpile", "bush"}
FIRES = {"fire", "bridgefire", "campfire"}

# map char -> (layer, object)
CHARS = {
    "&": ("mid", "tree"), "O": ("mid", "oak"), "~": ("mid", "water"), "D": ("mid", "deep"),
    "=": ("ground", "bridge"), "%": ("mid", "bush"), "B": ("mid", "bramble"), "F": ("mid", "campfire"),
    "f": ("mid", "fire"), "W": ("mid", "well"), "h": ("mid", "hen"), "L": ("mid", "logpile"),
    "#": ("mid", "rock"), "$": ("mid", "hedge"), "@": ("mid", "stone"), "!": ("mid", "tombstone"),
    "|": ("mid", "torch"), ",": ("ground", "grass"), ";": ("ground", "moss"), ":": ("ground", "dirt"),
    "_": ("ground", "sand"), "+": ("ground", "bones"), "*": ("ground", "mushroom"), "^": ("ground", "skull"),
}


class State:
    __slots__ = ("w", "h", "mid", "ground", "ids", "pos", "logs", "bucket", "t", "events", "alive", "hens",
                 "next_id", "dead")

    def copy(self):
        s = State.__new__(State)
        s.w, s.h = self.w, self.h
        s.mid, s.ground, s.ids = list(self.mid), list(self.ground), list(self.ids)
        s.pos, s.logs, s.bucket, s.t = self.pos, self.logs, self.bucket, self.t
        s.events = list(self.events)
        s.alive, s.hens, s.next_id, s.dead = self.alive, self.hens, self.next_id, self.dead
        return s

    # ------------------------------------------------------------------ helpers
    def idx(self, x, y):
        return y * self.w + x

    def inside(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def top(self, c):
        """Top-most object name at cell index c (None = empty); 'avatar' when the lumberjack is there."""
        if self.alive and c == self.pos:
            return "avatar"
        return self.mid[c] or self.ground[c]

    def key(self):
        ev = tuple(sorted((t - self.t, c, k, d, i) for t, c, k, d, i in self.events))
        # ids only matter through events; renumber them relative to the events for de-duplication
        return (self.pos, self.logs, self.bucket, tuple(self.mid), tuple(self.ground), self.alive,
                tuple((t, c, k, d) for t, c, k, d, _ in ev))

    # ------------------------------------------------------------------ fire
    def _light(self, c, kind):
        self.next_id += 1
        self.mid[c] = kind
        self.ids[c] = self.next_id
        for d in DIRS:
            self.events.append((self.t + S, c, "spread", d, self.next_id))
        if kind != "campfire":
            self.events.append((self.t + B, c, "burnout", 0, self.next_id))

    def _remove_fire(self, c):
        self.mid[c] = None
        self.ids[c] = 0

    def _run_events(self):
        due = [e for e in self.events if e[0] <= self.t]
        if not due:
            return
        self.events = [e for e in self.events if e[0] > self.t]
        for t, c, kind, d, oid in sorted(due):
            if self.ids[c] != oid or self.mid[c] not in FIRES:
                continue  # source object gone
            if kind == "burnout":
                if self.mid[c] == "fire":
                    self._remove_fire(c)
                elif self.mid[c] == "bridgefire":
                    self._remove_fire(c)
                    self.mid[c] = "water"
                continue
            x, y = c % self.w, c // self.w
            dx, dy = DIRS[d]
            nx, ny = x + dx, y + dy
            if not self.inside(nx, ny):
                continue  # _boundary: no rule, the fire stops trying this way
            n = self.idx(nx, ny)
            top = self.top(n)
            if top in FLAMMABLE:
                self.mid[n] = None
                self._light(n, "fire")
            elif top == "bridge":
                self.ground[n] = None
                self._light(n, "bridgefire")
            else:
                self.events.append((self.t + S, c, "spread", d, oid))

    # ------------------------------------------------------------------ player
    def step(self, action, d):
        """Apply one player action (name, dir 1-4; dir 0 = no-op) and advance one tick."""
        s = self
        if s.alive and d in DIRS:
            x, y = s.pos % s.w, s.pos // s.w
            dx, dy = DIRS[d]
            nx, ny = x + dx, y + dy
            if s.inside(nx, ny):
                c = s.idx(nx, ny)
                top = s.top(c)
                mid = s.mid[c]
                if action == "move":
                    if top in WALKABLE:
                        s.pos = c
                    elif top == "hen":
                        s.mid[c] = None
                        s.hens -= 1
                        s.pos = c
                    elif top in FIRES:
                        s.alive = False
                elif action == "chop":
                    if top == "tree" and s.logs < CAP:
                        s.mid[c], s.ground[c] = None, "stump"
                        s.logs += 1
                    elif top == "oak":
                        s.mid[c] = "oakcut"
                    elif top == "oakcut" and s.logs < CAP:
                        s.mid[c], s.ground[c] = None, "stump"
                        s.logs += 1
                    elif top == "logpile" and s.logs < CAP:
                        s.mid[c] = None
                        s.logs += 1
                    elif top == "bush":
                        s.mid[c] = None
                elif action == "build":
                    if top == "water" and s.logs >= 1:
                        s.mid[c], s.ground[c] = None, "bridge"
                        s.logs -= 1
                    elif top == "deep" and s.logs >= 2:
                        s.mid[c], s.ground[c] = None, "bridge"
                        s.logs -= 2
                    elif top in WALKABLE and s.logs >= 1:
                        s.mid[c] = "logpile"
                        s.logs -= 1
                elif action == "bucket":
                    if s.bucket == 0 and top in ("water", "deep", "well"):
                        s.bucket = 1
                    elif s.bucket == 1 and top in FIRES:
                        kind = mid
                        s._remove_fire(c)
                        s.bucket = 0
                        if kind == "bridgefire":
                            s.ground[c] = "bridge"
        s.t += 1
        s._run_events()
        return s


def load(rows: list[str], ground: list[list[str]] | None = None) -> State:
    s = State.__new__(State)
    s.h, s.w = len(rows), len(rows[0])
    s.mid, s.ground, s.ids = [None] * (s.w * s.h), [None] * (s.w * s.h), [0] * (s.w * s.h)
    s.logs = s.bucket = s.t = 0
    s.events, s.alive, s.hens, s.next_id, s.dead = [], True, 0, 0, False
    s.pos = -1
    fires = []
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            c = y * s.w + x
            g = ground[y][x] if ground else ""
            if g:
                s.ground[c] = CHARS[g][1]
            if ch == "A":
                s.pos = c
            elif ch in CHARS:
                layer, name = CHARS[ch]
                if layer == "ground":
                    s.ground[c] = name
                else:
                    s.mid[c] = name
                    if name == "hen":
                        s.hens += 1
                    if name in FIRES:
                        fires.append(c)
    for c in fires:  # InitialActions of the level's fires (order irrelevant: all due at the same ticks)
        s._light(c, s.mid[c])
    return s


# ---------------------------------------------------------------------------------------------- planner
def bfs(s: State, start, goal_cells, avoid_fire=True):
    """Shortest walk from start to any cell in goal_cells (cells must be walkable). Returns dir list."""
    if start in goal_cells:
        return []
    prev = {start: None}
    q = [start]
    for c in q:
        x, y = c % s.w, c // s.w
        for d, (dx, dy) in DIRS.items():
            nx, ny = x + dx, y + dy
            if not s.inside(nx, ny):
                continue
            n = s.idx(nx, ny)
            if n in prev or s.mid[n] is not None:
                continue
            if avoid_fire and _near_fire(s, n):
                continue
            prev[n] = (c, d)
            if n in goal_cells:
                path = []
                while prev[n]:
                    n, d = prev[n]
                    path.append(d)
                return path[::-1]
            q.append(n)
    return None


def _near_fire(s, c):
    return False  # fire never harms by spreading (only walking into it kills); kept as a hook


def dist_map(s: State, start):
    dist = {start: 0}
    q = [start]
    for c in q:
        x, y = c % s.w, c // s.w
        for dx, dy in DIRS.values():
            nx, ny = x + dx, y + dy
            if s.inside(nx, ny):
                n = s.idx(nx, ny)
                if n not in dist and s.mid[n] is None:
                    dist[n] = dist[c] + 1
                    q.append(n)
    return dist


RELAX = {"water": 6, "deep": 10, "tree": 4, "oak": 6, "oakcut": 4, "bush": 2, "bramble": 14, "logpile": 3,
         "campfire": 10, "fire": 6, "bridgefire": 6, "hen": 1}


def doomed(s: State):
    """Flammable cells (and bridges) connected to a burning fire through flammable cells: fire will get there."""
    q = [c for c, m in enumerate(s.mid) if m in FIRES]
    seen = set(q)
    for c in q:
        x, y = c % s.w, c // s.w
        for dx, dy in DIRS.values():
            nx, ny = x + dx, y + dy
            if s.inside(nx, ny):
                n = s.idx(nx, ny)
                if n not in seen and (s.mid[n] in FLAMMABLE or (s.mid[n] is None and s.ground[n] == "bridge")):
                    seen.add(n)
                    q.append(n)
    return seen


def relaxed(s: State, start):
    """Dijkstra from start where obstacles that the lumberjack can remove cost extra (cells fire will clear
    cost little)."""
    burn = doomed(s)
    dist = {start: 0}
    prev = {start: None}
    pq = [(0, start)]
    while pq:
        dd, c = heapq.heappop(pq)
        if dd > dist[c]:
            continue
        x, y = c % s.w, c // s.w
        for dx, dy in DIRS.values():
            nx, ny = x + dx, y + dy
            if not s.inside(nx, ny):
                continue
            n = s.idx(nx, ny)
            m = s.mid[n]
            step = 1 if m is None else RELAX.get(m)
            if m in FLAMMABLE and n in burn:
                step = 2
            if step is None:
                continue
            nd = dd + step
            if nd < dist.get(n, 1 << 30):
                dist[n], prev[n] = nd, c
                heapq.heappush(pq, (nd, n))
    return dist, prev


def heuristic(s: State, hints=None):
    if not s.alive:
        return 1 << 20
    extra = -2 * s.logs  # logs in hand are progress (they become bridges / fuses)
    fuse = (hints or {}).get("fuse", ())
    if fuse and any(m == "bramble" for m in s.mid):
        burn = doomed(s)
        if not any(s.mid[c] == "bramble" and c in burn for c in range(len(s.mid))):
            extra += 6 * sum(1 for c in fuse if s.mid[c] not in ("logpile",) + tuple(FIRES))
    if not any(m == "hen" for m in s.mid):
        return 0
    # wood budget: logs the relaxed routes still need vs. wood that is not doomed to burn
    burn = doomed(s)
    rc = set(route_cells(s, bridges=True))
    need = sum(2 if s.mid[c] == "deep" else 1 for c in rc if s.mid[c] in ("water", "deep"))
    need += sum(1 for c in rc if (s.mid[c] is None and s.ground[c] == "bridge" and c in burn) or s.mid[c] == "bridgefire")
    if fuse and any(s.mid[c] == "bramble" and c not in burn for c in range(len(s.mid))):
        need += sum(1 for c in fuse if s.mid[c] is None)
    have = s.logs + sum(1 for c, m in enumerate(s.mid) if m in ("tree", "oak", "oakcut", "logpile") and c not in burn)
    extra += 40 * max(0, need - have)
    return max(0, _route_cost(s) + extra)


def _route_cost(s: State):
    hens = [c for c, m in enumerate(s.mid) if m == "hen"]
    if not hens:
        return 0
    total, cur, left = 0, s.pos, set(hens)
    while left:
        dist, _ = relaxed(s, cur)
        best = min(left, key=lambda c: dist.get(c, 1 << 20))
        total += dist.get(best, 1 << 20)
        cur = best
        left.discard(best)
    return total


def route_cells(s: State, bridges=False):
    """Cells on the relaxed routes from the lumberjack to each remaining hen (excluding walkable ones;
    with bridges=True also the bridge cells on them)."""
    dist, prev = relaxed(s, s.pos)
    out = []
    for c, m in enumerate(s.mid):
        if m != "hen" or c not in dist:
            continue
        n = prev[c]
        chain = []
        while n is not None and n != s.pos:
            if s.mid[n] is not None or (bridges and s.ground[n] == "bridge"):
                chain.append(n)
            n = prev[n]
        out.extend(reversed(chain))
    return out


def adjacent_reach(s: State, dist, target):
    """(walk cell, dir) pairs next to target that are reachable, nearest first."""
    x, y = target % s.w, target // s.w
    out = []
    for d, (dx, dy) in DIRS.items():
        ax, ay = x - dx, y - dy
        if s.inside(ax, ay) and s.idx(ax, ay) in dist:
            out.append((dist[s.idx(ax, ay)], s.idx(ax, ay), d))
    return sorted(out)


def macros(s: State, hints):
    """Candidate macro actions: (label, [(action, dir), ...])."""
    dist = dist_map(s, s.pos)
    out = []

    def go(target, action, label):
        opts = adjacent_reach(s, dist, target)
        if not opts:
            return
        _, cell, d = opts[0]
        path = bfs(s, s.pos, {cell})
        if path is None:
            return
        out.append((label, [("move", p) for p in path] + [(action, d)]))

    route = route_cells(s)
    route_set = set(route)
    for c, m in enumerate(s.mid):
        if m == "hen":
            go(c, "move", f"hen@{c}")
    # bridges / blockers on the relaxed route
    for c in route:
        m = s.mid[c]
        if m in ("water", "deep") and s.logs >= (2 if m == "deep" else 1):
            go(c, "build", f"bridge@{c}")
        elif m in ("tree", "oak", "oakcut", "bush", "logpile"):
            go(c, "chop", f"clear@{c}")
        elif m in FIRES and s.bucket:
            go(c, "bucket", f"douse@{c}")
    # gather wood: nearest choppable things (not those on the route, already offered)
    if s.logs < CAP:
        wood = sorted((dd, c) for c, m in enumerate(s.mid)
                      if m in ("tree", "oak", "oakcut", "logpile") and c not in route_set
                      for dd in [dist_to(s, dist, c)] if dd is not None)
        for _, c in wood[:hints.get("wood", 3)]:
            go(c, "chop", f"chop@{c}")
    # bucket
    if s.bucket == 0 and any(m in ("fire", "bridgefire") for m in s.mid):
        srcs = sorted((dd, c) for c, m in enumerate(s.mid) if m in ("water", "deep", "well")
                      for dd in [dist_to(s, dist, c)] if dd is not None)
        for _, c in srcs[:2]:
            go(c, "bucket", f"fill@{c}")
    if s.bucket == 1:
        bridges = [c for c in route_cells(s, bridges=True) if s.mid[c] is None and s.ground[c] == "bridge"]
        for c, m in enumerate(s.mid):
            if m not in FIRES or c in route_set:
                continue
            near = any(abs(c % s.w - b % s.w) + abs(c // s.w - b // s.w) <= 2 for b in bridges)
            if near or c in hints.get("douse", ()):
                go(c, "bucket", f"douse@{c}")
    # firebreaks / extra clearing (level hint)
    for c in hints.get("chop", ()):
        if s.mid[c] in ("tree", "oak", "oakcut", "bush", "logpile"):
            go(c, "chop", f"break@{c}")
    # fuse cells (level hint): drop a log pile
    if s.logs >= 1:
        for c in hints.get("fuse", ()):
            if s.mid[c] is None and s.pos != c and c in dist:
                go(c, "build", f"pile@{c}")
    # standing on a fuse cell blocks the fire: step off it
    if s.pos in hints.get("fuse", ()):
        for d in DIRS:
            out.append((f"step{d}", [("move", d)]))
    # waiting lets fires do their work
    if any(m in ("fire", "bridgefire") for m in s.mid) or _camp_fed(s):
        out.append(("wait", [("move", 0)] * S))
    return out


def _camp_fed(s):
    for c, m in enumerate(s.mid):
        if m == "campfire":
            x, y = c % s.w, c // s.w
            for dx, dy in DIRS.values():
                if s.inside(x + dx, y + dy):
                    n = s.idx(x + dx, y + dy)
                    if s.mid[n] in FLAMMABLE or (s.mid[n] is None and s.ground[n] == "bridge"):
                        return True
    return False


def dist_to(s, dist, target):
    opts = adjacent_reach(s, dist, target)
    return opts[0][0] if opts else None


def run(s: State, steps):
    s = s.copy()
    for a, d in steps:
        s.step(a, d)
        if not s.alive:
            return None
    return s


def solve(s0: State, hints=None, max_expand=60000, weight=2.0):
    """Weighted A* over macro actions, optionally through intermediate stages (hints["stages"]: list of
    predicates State -> bool reached in order; the last stage is always "every hen rescued").
    Returns (list of (action, dir) or None, expansions)."""
    hints = hints or {}
    plan, total, s = [], 0, s0
    for goal in list(hints.get("stages", ())) + [lambda st: st.hens == 0]:
        part, n, s = _astar(s, hints, goal, max_expand, weight)
        total += n
        if part is None:
            return None, total
        plan += part
    return plan, total


def _astar(s0, hints, goal, max_expand, weight):
    tie = itertools.count()
    start = s0.copy()
    pq = [(weight * heuristic(start, hints), next(tie), 0, start, [])]
    seen = {start.key(): 0}
    expanded = 0
    while pq and expanded < max_expand:
        _, _, g, s, plan = heapq.heappop(pq)
        if goal(s):
            return plan, expanded, s
        expanded += 1
        for label, steps in macros(s, hints):
            n = run(s, steps)
            if n is None:
                continue
            k = n.key()
            ng = g + len(steps)
            if seen.get(k, 1 << 30) <= ng:
                continue
            seen[k] = ng
            heapq.heappush(pq, (ng + weight * heuristic(n, hints), next(tie), ng, n, plan + steps))
    return None, expanded, None


def walk_only(s: State):
    """Hens reachable by walking alone (no chop / build / bucket, fires frozen)."""
    dist = dist_map(s, s.pos)
    return [c for c, m in enumerate(s.mid) if m == "hen" and dist_to(s, dist, c) is not None]
