#!/usr/bin/env python3
"""OOM Killer: bufo, the scheduler, keeps training jobs from eating all the VRAM. A new "meme" game.

Rules (the GDY below; `Sim` mirrors them exactly and is checked against the engine with `--diff`):
  * The floor is VRAM (mayo-white memory cells). Each training job (a coloured blob around its core)
    grows: every job cell tries each of its 4 neighbours every G ticks and fills free VRAM. Jobs of the
    same colour share a phase (job colour k grows on ticks = k mod G), so no two colours ever grow in the
    same tick, nothing grows on a tick a job finishes, and the outcome never depends on event order.
  * Every job has a lifetime by colour (LIFE): when it is up the whole job (core and cells) finishes and
    frees its memory. Win when every job has finished.
  * Lose (OOM!) as soon as any job grows onto the red OOM line.
  * Bufo walks over free VRAM and the OOM line (a job can't grow into the cell bufo stands on).
    `kill` (Space, directional) frees one job cell (a core can't be killed: it is the process itself).
    `checkpoint` (E, directional) drops a checkpoint (a floppy) on free VRAM: a permanent wall jobs can't
    grow through. Pick up checkpoints from floppy stacks by walking into them. `nap` (Z) waits.
Determinism: exec timers only (no Randomize / Probability / Triggers).

Layout characters: W wall  R server rack (wall)  A bufo  1 2 3 4 job cores (colour / lifetime)
  L OOM line  F floppy stack (+1 checkpoint)  K placed checkpoint  x cable (floor)  . free VRAM
Validation per level: rectangular, one bufo; cores can reach the OOM line (idle bufo loses); the rollout
planner finds a winning run, written to apps/web/src/games/solutions/oom.json.
"""
from __future__ import annotations

import collections
import heapq
import json
import random
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

GID = "oom"
HERE = Path(__file__).resolve().parent
SOLUTIONS = C.ROOT / "studio/griddly-demo/apps/web/src/games/solutions" / f"{GID}.json"
DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
G = 5                                       # growth period (lifetimes are multiples of G, phases 1..4:
                                            # no job ever grows on the tick another one finishes)
LIFE = {1: 60, 2: 100, 3: 140, 4: 180}      # job lifetime by colour
PHASE = {1: 1, 2: 2, 3: 3, 4: 4}            # first growth tick by colour (then every G)
COLOURS = {1: ("orange", (255, 140, 30)), 2: ("violet", (150, 90, 230)), 3: ("teal", (40, 190, 170)),
           4: ("pink", (240, 110, 170))}
JOBS = [f"job{k}" for k in LIFE]
CORES = [f"core{k}" for k in LIFE]
FLOOR = ["cable"]
SOLID = ["wall", "rack", "checkpoint", "floppy", "oomhit"]
ALL = FLOOR + ["oomline"] + SOLID + JOBS + CORES + ["bufo"]
CHARS = {"W": "wall", "R": "rack", "L": "oomline", "F": "floppy", "K": "checkpoint", "x": "cable",
         **{str(k): f"core{k}" for k in LIFE}}


def lst(names):
    return "[" + ", ".join(names) + "]"


# ----------------------------------------------------------------------------------------------
# Art (original)
# ----------------------------------------------------------------------------------------------
def art():
    P = M.PALETTE
    a = {}
    for k, (cname, rgb) in COLOURS.items():
        hi = tuple(min(255, v + 70) for v in rgb) + (255,)
        dark = tuple(max(0, v - 70) for v in rgb) + (255,)
        base = rgb + (255,)
        px = M.blank()
        M.ellipse(px, 12, 12.5, 10.5, 10, base, hi, dark)
        for x, y in ((7, 8), (8, 8), (7, 9)):
            M.put(px, x, y, P["white"])
        # a tiny loss curve on the blob
        for i, x in enumerate(range(8, 17)):
            M.put(px, x, 11 + int(5 * (1 - (i / 8) ** 0.4)), dark)
        M.outline(px)
        a[f"job{k}"] = M.save(f"oom-job-{cname}", px)
        px = M.blank()
        M.ellipse(px, 12, 12.5, 11.3, 10.8, dark)
        M.ellipse(px, 12, 12.5, 10, 9.5, base, hi, dark)
        M.rect(px, 7, 8, 16, 16, P["shroud"])  # a little GPU die in the nucleus
        M.rect(px, 7, 8, 16, 8, P["shroud_hi"])
        M.rect(px, 9, 10, 14, 14, P["gold"])
        M.rect(px, 10, 11, 13, 13, rgb + (255,))
        M.outline(px)
        a[f"core{k}"] = M.save(f"oom-core-{cname}", px)
    # checkpoint: a floppy disk
    def floppy(px, ox, oy, s=1.0):
        M.rect(px, ox, oy, ox + 13, oy + 13, (40, 70, 160, 255))
        M.rect(px, ox + 3, oy, ox + 10, oy + 4, P["steel_hi"])
        M.rect(px, ox + 7, oy + 1, ox + 8, oy + 3, P["steel_dark"])
        M.rect(px, ox + 2, oy + 7, ox + 11, oy + 13, P["white"])
        M.rect(px, ox + 3, oy + 9, ox + 10, oy + 9, P["mistral_orange"])
        M.rect(px, ox + 3, oy + 11, ox + 8, oy + 11, (150, 150, 160, 255))
    px = M.blank()
    floppy(px, 5, 4)
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=21, rx=8)
    a["checkpoint"] = M.save("oom-checkpoint", px)
    px = M.blank()
    for i, (ox, oy) in enumerate(((2, 9), (8, 6), (5, 2))):
        floppy(px, ox, oy)
    M.outline(px)
    a["floppy"] = M.save("oom-floppies", px)
    # OOM line: red / yellow hazard stripes on the floor
    px = M.blank()
    for y in range(24):
        for x in range(24):
            px[y][x] = (210, 30, 30, 255) if ((x + y) // 4) % 2 else (250, 200, 30, 255)
    M.rect(px, 0, 0, 23, 0, (120, 10, 10, 255))
    M.rect(px, 0, 23, 23, 23, (120, 10, 10, 255))
    a["oomline"] = M.save("oom-line", px)
    # OOM explosion
    load = lambda rel: C.load_png(C.IMAGES_DIR / rel)[2]
    px = M.copy(load(a["oomline"]))
    M.paste(px, load(M.gpu_fire_frames()[0]))
    a["oomhit"] = M.save("oom-hit", px)
    # free VRAM floor: mayo-white memory cell tile
    px = M.blank((242, 236, 210, 255))
    M.rect(px, 0, 0, 23, 0, (255, 252, 236, 255))
    M.rect(px, 0, 0, 0, 23, (255, 252, 236, 255))
    M.rect(px, 0, 23, 23, 23, (206, 196, 160, 255))
    M.rect(px, 23, 0, 23, 23, (206, 196, 160, 255))
    for x in range(4, 21, 4):
        M.put(px, x, 3, P["gold"])
        M.put(px, x, 20, P["gold"])
    a["floor"] = M.save("oom-vram", px)
    a["rack"] = M.rack_frames()
    a["cable"] = M.cable()
    a["bufo"] = M.bufo("plain")
    M.datacenter_wall16()
    return a


# ----------------------------------------------------------------------------------------------
# Levels
# ----------------------------------------------------------------------------------------------
LEVELS = [
    ("Hello World Job", "One job is growing toward the OOM line. Grab a floppy and checkpoint the corridor (E).", [
        "WWWWWWWWWWWWWWWWWWWW",
        "W........W.........W",
        "W....1...W...F.....W",
        "W........W.........W",
        "W........W....A....W",
        "WWWWW.WWWW.........W",
        "W....x.............W",
        "W..................W",
        "WLLLLLLLLLLLLLLLLLLW",
        "WWWWWWWWWWWWWWWWWWWW",
    ]),
    ("kill -9", "No floppies. Block the 2-wide door with your body and kill -9 (Space) whatever squeezes past.", [
        "WWWWWWWWWWWWWWWWWWWWWW",
        "W....................W",
        "W.........2..........W",
        "W....................W",
        "WWWWWWWWW..WWWWWWWWWWW",
        "W....................W",
        "W..A.......x.........W",
        "W..........x.........W",
        "WLLLLLLLLLLLLLLLLLLLLW",
        "WWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Checkpoint Every Epoch", "Three jobs, three doors, three floppies. Seal the doors before the jobs get there.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWW",
        "W...1...W...2...W...3...W",
        "W.......W.......W.......W",
        "W.......W.......W.......W",
        "W.......W.......W.......W",
        "W.......W.......W.......W",
        "W.......W.......W.......W",
        "WWWW.WWWWWWW.WWWWWWW.WWWW",
        "W.......................W",
        "W.........FAF...........W",
        "W..........F............W",
        "W.......................W",
        "WLLLLLLLLLLLLLLLLLLLLLLLW",
        "WWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Memory Leak", "A long-running pink job has three doors; you have two floppies. Hold the third yourself.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWWW",
        "W.....4......W.....1.....W",
        "W............W...........W",
        "W............W...........W",
        "W............W...........W",
        "W............W...........W",
        "WRRR.RRRR.RRRWRRRRR.RRRRRW",
        "W........................W",
        "W.......FAF..............W",
        "W........................W",
        "W........................W",
        "WLLLLLLLLLLLLLLLLLLLLLLLLW",
        "WWWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("OOM Killer", "Four jobs, OOM lines above and below, four floppies. Survive until every job finishes.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
        "WLLLLLLLLLLLLLLLLLLLLLLLLLLLLW",
        "W............................W",
        "WRRRRRRRRRRR.RRRRR.RRRRRRRRRRW",
        "W1.........W.......W........3W",
        "W..........W.......W.........W",
        "W..........W...F...W.........W",
        "W...............A............W",
        "W..........W..FFF..W.........W",
        "W..........W.......W.........W",
        "W2.........W.......W........4W",
        "WRRRRRRRRRRR.RRRRR.RRRRRRRRRRW",
        "W............................W",
        "WLLLLLLLLLLLLLLLLLLLLLLLLLLLLW",
        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
]


# ----------------------------------------------------------------------------------------------
# Simulator (mirrors the GDY)
# ----------------------------------------------------------------------------------------------
class Sim:
    __slots__ = ("w", "h", "mid", "line", "cable", "oid", "pos", "ckpts", "t", "events", "cores", "oom", "result",
                 "next_id", "seq", "end")

    def copy(self):
        s = Sim.__new__(Sim)
        s.w, s.h, s.line, s.cable = self.w, self.h, self.line, self.cable
        s.mid, s.oid, s.events = list(self.mid), list(self.oid), list(self.events)
        s.pos, s.ckpts, s.t, s.cores, s.oom, s.result = self.pos, self.ckpts, self.t, self.cores, self.oom, self.result
        s.next_id, s.seq, s.end = self.next_id, self.seq, self.end
        return s

    def _push(self, t, c, kind, d):
        self.seq += 1
        self.events.append((t, self.seq, c, kind, d, self.oid[c]))

    def _create(self, c, name, initial=False):
        self.next_id += 1
        self.mid[c], self.oid[c] = name, self.next_id
        if name.startswith(("job", "core")):
            k = int(name[-1])
            first = PHASE[k] if initial else G
            for d in DIRS:
                self._push(self.t + first, c, "spread", d)
            self._push(LIFE[k], c, "expire", 0)

    def free(self, c):
        """Free VRAM for growth / checkpoints (floor, cable; not the line, not bufo)."""
        return self.mid[c] is None and c not in self.line and c != self.pos

    def _run_events(self):
        due = sorted(e for e in self.events if e[0] <= self.t)
        if not due:
            return
        self.events = [e for e in self.events if e[0] > self.t]
        for t, _, c, kind, d, oid in due:
            if self.oid[c] != oid or self.mid[c] is None:
                continue
            m = self.mid[c]
            k = int(m[-1])
            if kind == "expire":
                self.mid[c], self.oid[c] = None, 0
                if m.startswith("core"):
                    self.cores -= 1
                continue
            if not self.t < LIFE[k] - 1:
                continue
            self._push(self.t + G, c, "spread", d)
            dx, dy = DIRS[d]
            n = c + dx + dy * self.w
            if n == self.pos or self.mid[n] is not None:
                continue
            if n in self.line:
                self.oom += 1
                self.mid[n], self.oid[n] = "oomhit", 0
                self.next_id += 1
                continue
            self._create(n, f"job{k}")

    def walkable(self, c):
        return self.mid[c] is None

    def step(self, action, d):
        if self.result:
            return self
        if d in DIRS and action in ("move", "kill", "checkpoint"):
            dx, dy = DIRS[d]
            n = self.pos + dx + dy * self.w
            m = self.mid[n]
            if action == "move":
                if m is None:
                    self.pos = n
                elif m == "floppy":
                    self.mid[n], self.oid[n] = None, 0
                    self.ckpts += 1
                    self.pos = n
            elif action == "kill":
                if m in JOBS:
                    self.mid[n], self.oid[n] = None, 0
            elif action == "checkpoint":
                if self.ckpts > 0 and m is None and n not in self.line:
                    self.ckpts -= 1
                    self.next_id += 1
                    self.mid[n], self.oid[n] = "checkpoint", self.next_id
        self.t += 1
        self._run_events()
        if self.oom > 0:
            self.result = "Lose"
        elif self.cores == 0:
            self.result = "Win"
        return self


def load(rows) -> Sim:
    s = Sim.__new__(Sim)
    s.h, s.w = len(rows), len(rows[0])
    n = s.w * s.h
    s.mid, s.oid = [None] * n, [0] * n
    line, cable = set(), set()
    s.ckpts = s.t = s.cores = s.oom = s.next_id = s.seq = 0
    s.events, s.result, s.pos, s.end = [], None, -1, 0
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            c = y * s.w + x
            if ch == "A":
                s.pos = c
            elif ch == "L":
                line.add(c)
            elif ch == "x":
                cable.add(c)
            elif ch in CHARS:
                name = CHARS[ch]
                s._create(c, name, initial=True)
                if name.startswith("core"):
                    s.cores += 1
                    s.end = max(s.end, LIFE[int(name[-1])])
    s.line, s.cable = frozenset(line), frozenset(cable)
    return s


# ----------------------------------------------------------------------------------------------
# Planner: rollouts over macro actions
# ----------------------------------------------------------------------------------------------
def growth_arrival(s: Sim, blocked=frozenset()):
    """Earliest tick each free cell / line cell can be filled by each job colour, ignoring bufo and kills.
    Returns ({cell: (tick, colour)}, earliest OOM (tick, colour) or None)."""
    best = {}
    pq = []
    pending = {}
    for t, _, c, kind, d, oid in s.events:  # next growth tick per job cell
        if kind == "spread" and s.oid[c] == oid and s.mid[c] and s.mid[c] != "oomhit":
            pending[c] = min(pending.get(c, 1 << 30), t)
    for c, t in pending.items():
        k = int(s.mid[c][-1])
        heapq.heappush(pq, (t, c, k))
    oom = None
    while pq:
        t, c, k = heapq.heappop(pq)
        if t >= LIFE[k] - 1:
            continue
        for dx, dy in DIRS.values():
            n = c + dx + dy * s.w
            if n in blocked or s.mid[n] is not None:
                continue
            if n in best and best[n][0] <= t:
                continue
            best[n] = (t, k)
            if n in s.line:
                if oom is None or t < oom[0]:
                    oom = (t, k)
                continue
            heapq.heappush(pq, (t + G, n, k))
    return best, oom


def path_to(s: Sim, goals: dict):
    if s.pos in goals:
        return [], s.pos
    prev = {s.pos: None}
    q = [s.pos]
    for c in q:
        for d, (dx, dy) in DIRS.items():
            n = c + dx + dy * s.w
            if n in prev or not s.walkable(n):
                continue
            prev[n] = (c, d)
            if n in goals:
                end = n
                path = []
                while prev[n]:
                    n, dd = prev[n]
                    path.append(dd)
                return path[::-1], end
            q.append(n)
    return None, None


def sides(s: Sim, c):
    out = {}
    for d, (dx, dy) in DIRS.items():
        n = c - dx - dy * s.w
        if s.walkable(n) or n == s.pos:
            out[n] = d
    return out


def oom_path(s: Sim, arrival, oom):
    """Cells on the growth path ending at the earliest OOM line cell (back-traced through arrival times)."""
    if oom is None:
        return []
    t0, k = oom
    end = next(c for c in s.line if c in arrival and arrival[c] == (t0, k))
    path = [end]
    c = end
    while True:
        t, _ = arrival[c]
        prev = None
        for dx, dy in DIRS.values():
            n = c + dx + dy * s.w
            if n in arrival and arrival[n] == (t - G, k) and n not in s.line:
                prev = n
                break
        if prev is None:
            break
        path.append(prev)
        c = prev
    return path


def do(s: Sim, log, act, d):
    s.step(act, d)
    if log is not None:
        log.append([act, d])


def guard(s: Sim, log):
    """Wait a tick, killing a neighbouring job cell if there is one (the one closest to an OOM line)."""
    best, bd = None, None
    for d, (dx, dy) in DIRS.items():
        n = s.pos + dx + dy * s.w
        if s.mid[n] in JOBS:
            dist = min(abs(n // s.w - l // s.w) + abs(n % s.w - l % s.w) for l in s.line)
            if bd is None or dist < bd:
                best, bd = d, dist
    if best is not None:
        do(s, log, "kill", best)
    else:
        do(s, log, "nap", 1)


def run_macro(s: Sim, macro, log=None):
    kind, target = macro
    if kind == "nap":
        guard(s, log)
        return True
    if kind == "stand":
        if s.pos == target:
            guard(s, log)
            return True
        path, _ = path_to(s, {target: 0})
    else:
        goals = sides(s, target)
        if not goals:
            return False
        path, _ = path_to(s, goals)
    if path is None:
        return False
    for d in path:
        if s.result:
            return True
        do(s, log, "move", d)
    if s.result or kind == "stand":
        return True
    d = sides(s, target).get(s.pos)
    if d is None:
        return False
    act = {"kill": "kill", "ckpt": "checkpoint", "floppy": "move"}[kind]
    if kind == "kill" and s.mid[target] not in JOBS:
        return False
    if kind == "ckpt" and (s.ckpts == 0 or not s.free(target)):
        return False
    do(s, log, act, d)
    return True


def doors(s: Sim):
    """Bottleneck cells: free cells with solid cells on two opposite sides (gaps in rack rows, corridors)."""
    solid = lambda c: s.mid[c] in ("wall", "rack", "checkpoint", "floppy")
    out = []
    for c in range(s.w, s.w * (s.h - 1)):
        if s.mid[c] is not None or c in s.line or c % s.w in (0, s.w - 1):
            continue
        if (solid(c - 1) and solid(c + 1)) or (solid(c - s.w) and solid(c + s.w)):
            out.append(c)
    return out


def candidates(s: Sim, width=6):
    arrival, oom = growth_arrival(s)
    out = []
    path = oom_path(s, arrival, oom)
    if oom is not None:
        if s.ckpts > 0:
            # doors jobs will reach (earliest first), then the path cells that delay the OOM most
            ds = sorted((arrival[c][0], c) for c in doors(s) if c in arrival and s.free(c))
            out += [("ckpt", c) for _, c in ds[:3]]
            scored = []
            for c in path:
                if c in s.line or not s.free(c):
                    continue
                _, o2 = growth_arrival(s, frozenset([c]))
                scored.append(((o2[0] if o2 else 1 << 20), c))
            scored.sort(reverse=True)
            out += [("ckpt", c) for _, c in scored[:2] if ("ckpt", c) not in out]
        # kill the job cells next to the growth front on the path
        front = []
        for c in reversed(path):
            for dx, dy in DIRS.values():
                n = c + dx + dy * s.w
                if s.mid[n] in JOBS and n not in front:
                    front.append(n)
        out += [("kill", c) for c in front[:3]]
        ds = sorted((arrival[c][0], c) for c in doors(s) if c in arrival and s.walkable(c))
        out += [("stand", c) for _, c in ds[:2]]
        near = set()
        for c in path:
            for dx, dy in list(DIRS.values()) + [(0, 0)]:
                n = c + dx + dy * s.w
                if s.walkable(n) and n not in s.line:
                    near.add(n)
        scored = []
        for c in near:
            _, o2 = growth_arrival(s, frozenset([c]))
            scored.append(((o2[0] if o2 else 1 << 20), c))
        scored.sort(reverse=True)
        out += [("stand", c) for _, c in scored[:2] if ("stand", c) not in out]
        out += [("stand", c) for c in path[-3:] if c not in s.line and s.walkable(c) and ("stand", c) not in out][:2]
    fl = [c for c, m in enumerate(s.mid) if m == "floppy"]
    if fl:
        dist = {}
        for c in fl:
            p, _ = path_to(s, sides(s, c))
            if p is not None:
                dist[c] = len(p)
        if dist:
            out.append(("floppy", min(dist, key=dist.get)))
    return out[:width + 3], oom


def greedy(s: Sim):
    cand, oom = candidates(s, 6)
    fl = [m for m in cand if m[0] == "floppy"]
    if oom is None:
        return fl[0] if fl else ("nap", None)
    slack = oom[0] - s.t
    arrival, _ = growth_arrival(s)
    for m in cand:
        if m[0] == "ckpt":
            p, _ = path_to(s, sides(s, m[1]))
            if p is not None and len(p) + 1 < arrival.get(m[1], (1 << 20,))[0] - s.t:
                return m
    if fl:
        p, _ = path_to(s, sides(s, fl[0][1]))
        if p is not None and 2 * len(p) + 4 < slack:
            return fl[0]
    for m in cand:
        if m[0] == "kill":
            return m
    for m in cand:
        if m[0] == "stand":
            return m
    return ("nap", None)


def score(s: Sim):
    if s.result == "Win":
        return -1e9 + s.t
    if s.result == "Lose":
        return 1e8 - 1000 * s.t
    _, oom = growth_arrival(s)
    slack = (oom[0] - s.t) if oom else 400
    return -min(slack, 400) * 50 - s.ckpts * 30


def rollout(s: Sim, horizon):
    end = s.t + horizon
    while not s.result and s.t < end:
        m = greedy(s)
        if not run_macro(s, m):
            run_macro(s, ("nap", None))
    return score(s)


PLANS = [(40, 6), (80, 8), (160, 8)]


def plan(rows, horizon=40, width=6, max_ticks=400):
    s = load(rows)
    log = []
    while not s.result and s.t < max_ticks:
        cand, _ = candidates(s, width)
        g = greedy(s)
        if g not in cand:
            cand.append(g)
        best, bv = None, None
        for m in cand + [("nap", None)]:
            t = s.copy()
            if not run_macro(t, m):
                continue
            v = rollout(t, horizon)
            if bv is None or v < bv:
                best, bv = m, v
        if best is None or not run_macro(s, best, log):
            run_macro(s, ("nap", None), log)
    return s, log


# ----------------------------------------------------------------------------------------------
# GDY
# ----------------------------------------------------------------------------------------------
def actions():
    face = "{set_tile: meta.facing}"
    free = lst(["_empty"] + FLOOR)
    grow = ""
    for k in LIFE:
        src = f"[job{k}, core{k}]"
        pre = f"Preconditions: [{{lt: [_steps, {LIFE[k] - 1}]}}]"
        again = f"{{exec: {{Action: grow, Delay: {G}}}}}"
        grow += (f"      - Src: {{Object: {src}, {pre}, Commands: [{again}, {{spawn: job{k}}}]}}\n"
                 f"        Dst: {{Object: {free}}}\n"
                 f"      - Src: {{Object: {src}, {pre}, Commands: [{again}, {{incr: oom}}]}}\n"
                 f"        Dst: {{Object: oomline, Commands: [{{change_to: oomhit}}]}}\n"
                 f"      - Src: {{Object: {src}, {pre}, Commands: [{again}]}}\n"
                 f"        Dst: {{Object: {lst(SOLID + JOBS + CORES + ['bufo'])}}}\n")
    born = "".join(f"      - Src: {{Object: job{k}, Commands: [{{set: [ttl, {LIFE[k]}]}}, {{sub: [ttl, _steps]}}, "
                   f"{{exec: {{Action: finish, ActionId: 1, Delay: ttl}}}}]}}\n        Dst: {{Object: job{k}}}\n"
                   for k in LIFE)
    finish = "".join(f"      - Src: {{Object: core{k}, Commands: [{{decr: cores}}, {{remove: true}}]}}\n"
                     f"        Dst: {{Object: core{k}}}\n"
                     f"      - Src: {{Object: job{k}, Commands: [{{remove: true}}]}}\n        Dst: {{Object: job{k}}}\n"
                     for k in LIFE)
    others = lambda used: lst([n for n in ALL + ["_empty"] if n not in used])
    return f'''
Actions:
{C.face_action("bufo")}
  # Jobs grow: every job cell tries each neighbour every {G} ticks (colour k on ticks = k mod {G}) and fills
  # free VRAM; reaching the OOM line is an out-of-memory crash. Growth stops 1 tick before the job ends.
  - Name: grow
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
{grow}
  # A new job cell schedules its own end: the job's lifetime is absolute (all its cells end together).
  - Name: born
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Born, VectorToDest: [0, 0]}}
    Behaviours:
{born}
  - Name: finish
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Finish, VectorToDest: [0, 0]}}
    Behaviours:
{finish}
  - Name: count_core
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {lst(CORES)}, Commands: [{{incr: cores}}]}}
        Dst: {{Object: {lst(CORES)}}}

  # Bufo walks on free VRAM and the OOM line; walking into a floppy stack picks up a checkpoint.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: bufo, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {lst(["_empty"] + FLOOR + ["oomline"])}}}
      - Src: {{Object: bufo, Commands: [{face}, {{incr: ckpts}}, {{mov: _dest}}]}}
        Dst: {{Object: floppy, Commands: [{{remove: true}}]}}
      - Src: {{Object: bufo, Commands: [{face}]}}
        Dst: {{Object: {others(["_empty"] + FLOOR + ["oomline", "floppy", "bufo"])}}}

  # kill -9: free one job cell (cores can't be killed).
  - Name: kill
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: bufo, Commands: [{face}, {{reward: 1}}]}}
        Dst: {{Object: {lst(JOBS)}, Commands: [{{remove: true}}]}}
      - Src: {{Object: bufo, Commands: [{face}]}}
        Dst: {{Object: {others(JOBS + ["bufo"])}}}

  # Drop a checkpoint on free VRAM: jobs can't grow through it.
  - Name: checkpoint
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: bufo
          Preconditions: [{{gt: [src.ckpts, 0]}}]
          Commands: [{face}, {{decr: ckpts}}, {{spawn: checkpoint}}]
        Dst: {{Object: {free}}}
      - Src: {{Object: bufo, Commands: [{face}]}}
        Dst: {{Object: {others(["_empty"] + FLOOR + ["bufo"])}}}

  - Name: nap
    InputMapping:
      Inputs:
        1: {{Description: Nap, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: bufo, Commands: [{{nop: true}}]}}
        Dst: {{Object: bufo}}
'''


def objects(a):
    objs = [
        C.obj("wall", "W", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("rack", "R", 1, C.imgs(a["rack"]), "[0.2, 0.2, 0.25]", "square", 0.95, tiles=2),
        C.obj("cable", "x", 0, C.img(a["cable"]), "[0.2, 0.4, 0.9]", "square", 0.4),
        C.obj("oomline", "L", 0, C.img(a["oomline"]), "[0.9, 0.1, 0.1]", "square", 0.9),
        C.obj("oomhit", None, 1, C.img(a["oomhit"]), "[1.0, 0.3, 0.0]", "circle", 1.0),
        C.obj("floppy", "F", 1, C.img(a["floppy"]), "[0.2, 0.3, 0.8]", "square", 0.7),
        C.obj("checkpoint", "K", 1, C.img(a["checkpoint"]), "[0.3, 0.4, 0.9]", "square", 0.8),
    ]
    for k in LIFE:
        col = "[" + ", ".join(f"{v / 255:.2f}" for v in COLOURS[k][1]) + "]"
        grow = "".join(f"      - Action: grow\n        ActionId: {d}\n        Delay: {G}\n" for d in DIRS)
        born = "    InitialActions:\n      - Action: born\n        ActionId: 1\n" + grow
        objs.append(C.obj(f"job{k}", None, 1, C.img(a[f"job{k}"]), col, "circle", 0.8,
                          "    Variables:\n      - Name: ttl\n        InitialValue: 0\n" + born))
        core = ("    InitialActions:\n      - Action: count_core\n        ActionId: 1\n"
                f"      - Action: finish\n        ActionId: 1\n        Delay: {LIFE[k]}\n"
                + "".join(f"      - Action: grow\n        ActionId: {d}\n        Delay: {PHASE[k]}\n" for d in DIRS))
        objs.append(C.obj(f"core{k}", str(k), 1, C.img(a[f"core{k}"]), col, "square", 0.9, core))
    objs.append(C.obj("bufo", "A", 1, C.imgs(a["bufo"]), "[0.4, 0.7, 0.25]", "triangle", 0.9,
                      "    Variables:\n      - Name: ckpts\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION, tiles=4))
    return "\nObjects:\n" + "".join(objs)


def header(a):
    variables = "".join(f"    - Name: {n}\n      InitialValue: 0\n" for n in ("cores", "oom"))
    return C.header("OOM Killer", f"""
        VRAM is mayo and the training jobs are hungry. Every job grows into free memory every {G} ticks.
        Bufo, the scheduler, kills job cells (cores are unkillable) and drops checkpoints (walls) to
        contain them. Jobs finish after their lifetime (orange {LIFE[1]}, violet {LIFE[2]}, teal {LIFE[3]},
        pink {LIFE[4]} ticks). Win when every job has finished; lose if any job reaches the red OOM line.
        """.replace("        ", ""), "bufo", ["eq: [cores, 0]"], ["gt: [oom, 0]"],
        background=a["floor"], variables=variables)


# ----------------------------------------------------------------------------------------------
# main / diff
# ----------------------------------------------------------------------------------------------
def validate(name, rows):
    C.check_rect(name, rows)
    assert sum(r.count("A") for r in rows) == 1, f"{name}: need one bufo"
    for r in rows:
        assert set(r) <= set(CHARS) | {".", "A"}, f"{name}: unknown chars in {r}"
    assert any(ch in "1234" for r in rows for ch in r), f"{name}: no jobs"


def main():
    a = art()
    parts = [header(a)]
    sols = []
    for i, (name, blurb, rows) in enumerate(LEVELS):
        validate(name, rows)
        parts.append(C.level_block(name, blurb, rows))
        t0 = time.time()
        idle = load(rows)
        while not idle.result:
            idle.step("nap", 1)
        assert idle.result == "Lose", f"{name}: doing nothing does not lose"
        best = None
        for hz, w in PLANS:
            s, log = plan(rows, hz, w)
            if s.result == "Win":
                best = (s, log)
                break
        assert best is not None, f"{name}: planner found no winning run"
        s, log = best
        r = load(rows)
        for act, d in log:
            r.step(act, d)
        assert r.result == "Win" and r.t == len(log), f"{name}: replay mismatch"
        sols.append({"seed": 1, "actions": log})
        jobs = sum(ch in "1234" for r_ in rows for ch in r_)
        floppies = sum(r_.count("F") for r_ in rows)
        used = sum(1 for x, _ in log if x == "checkpoint")
        kills = sum(1 for x, _ in log if x == "kill")
        print(f"  L{i} {name:24s} {len(rows[0])}x{len(rows)} jobs={jobs} floppies={floppies} ends at t={idle.end}  "
              f"idle: OOM at t={idle.t}  planner: Win in {len(log)} steps ({kills} kills, {used} checkpoints, "
              f"{time.time() - t0:.1f}s)", file=sys.stderr)
    gdy = "".join(parts) + actions() + objects(a)
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps({"format": 2, "levels": sols}))
    C.emit(GID, gdy, {
        "name": "OOM Killer",
        "tagline": "VRAM is mayo. The jobs are hungry. Bufo schedules.",
        "description": "Training jobs grow through the VRAM like blobs of mayo. Bufo, the scheduler, kills job "
                       "cells and drops checkpoints to wall them in until each job finishes - one blob on the red "
                       "OOM line and the whole cluster crashes.",
        "tags": ["meme", "action", "containment", "strategy"],
        "difficulty": "medium",
        "original": "",
        "accent": "#ef4444",
        "icon": "💥",
        "controls": {"move": "move", "extra": [
            {"key": " ", "label": "Kill", "action": "kill", "directional": True},
            {"key": "e", "label": "Checkpoint", "action": "checkpoint", "directional": True},
            {"key": "z", "label": "Nap", "action": "nap", "directional": False}]},
        "hud": [{"label": "Jobs running", "kind": "global", "variable": "cores", "tone": "violet"},
                {"label": "Checkpoints", "kind": "var", "object": "bufo", "variable": "ckpts", "tone": "sky"},
                {"label": "Floppies left", "kind": "count", "object": "floppy", "tone": "emerald"},
                {"label": "Orange cells", "kind": "count", "object": "job1", "tone": "orange"},
                {"label": "Violet cells", "kind": "count", "object": "job2", "tone": "fuchsia"}],
        "win_text": "Every job finished. No OOM today!",
        "lose_text": "CUDA out of memory.",
    })
    if "--diff" in sys.argv:
        diff()


def snapshot(s: Sim):
    objs = {}
    for c, m in enumerate(s.mid):
        if m and m not in ("wall", "rack"):
            objs.setdefault(m, []).append([c % s.w, c // s.w])
    objs["bufo"] = [[s.pos % s.w, s.pos // s.w]]
    for k in objs:
        objs[k].sort()
    return {"objs": objs, "vars": {"ckpts": s.ckpts}, "globals": {"cores": s.cores, "oom": s.oom}, "term": s.result}


def diff(n_random=6, length=200):
    rng = random.Random(11)
    sols = json.loads(SOLUTIONS.read_text())["levels"]
    jobs = []
    for i in range(len(LEVELS)):
        jobs.append({"level": i, "actions": sols[i]["actions"]})
        for _ in range(n_random):
            acts = []
            for _ in range(length):
                r = rng.random()
                acts.append(["move", rng.randint(1, 4)] if r < 0.6 else ["kill", rng.randint(1, 4)] if r < 0.85
                            else ["checkpoint", rng.randint(1, 4)] if r < 0.95 else ["nap", 1])
            jobs.append({"level": i, "actions": acts})
    keep = [n for n in ALL if n not in ("wall", "rack", "cable", "oomline")]
    for j in jobs:
        j["keep"], j["vars"] = keep, ["ckpts"]
    with tempfile.TemporaryDirectory() as td:
        jp, op = Path(td) / "jobs.json", Path(td) / "out.json"
        jp.write_text(json.dumps(jobs))
        subprocess.run(["node", str(HERE / "meme_dump.mjs"), str(C.GAMES_DIR / f"{GID}.yaml"), str(jp), str(op)],
                       check=True, stdout=subprocess.DEVNULL)
        out = json.loads(op.read_text())
    bad = 0
    for job, states in zip(jobs, out):
        s = load(LEVELS[job["level"]][2])
        for t, eng in enumerate(states):
            if t > 0:
                s.step(*job["actions"][t - 1])
            mine = snapshot(s)
            mine["objs"] = {k: v for k, v in mine["objs"].items() if k not in ("cable",)}
            if mine != eng:
                bad += 1
                keys = sorted(set(mine["objs"]) | set(eng["objs"]))
                delta = {k: (mine["objs"].get(k), eng["objs"].get(k)) for k in keys
                         if mine["objs"].get(k) != eng["objs"].get(k)}
                print(f"  DIFF level {job['level']} step {t} {job['actions'][t - 1] if t else None}: "
                      f"{mine['vars']}/{eng['vars']} {mine['globals']}/{eng['globals']} {mine['term']}/{eng['term']} "
                      f"{delta}", file=sys.stderr)
                break
    total = sum(len(s) for s in out)
    print(f"diff: {len(jobs)} runs, {total} states compared, {bad} mismatching runs", file=sys.stderr)
    if bad:
        sys.exit(1)


if __name__ == "__main__":
    main()
