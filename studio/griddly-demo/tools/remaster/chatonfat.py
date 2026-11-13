#!/usr/bin/env python3
"""Le Chaton Fat: the fat datacenter cat keeps the GPUs from melting until the training run finishes.
A new "meme" game, not a port.

Rules (the GDY below; `Sim` in this file mirrors them exactly and is checked against the engine with
`--diff`):
  * GPUs heat up on fixed timers: cool -(TC ticks)-> warm -(TW)-> hot -(TH)-> ON FIRE.
  * A burning GPU (or server rack) tries each of its 4 neighbours every S ticks: cool/warm/hot GPUs and
    server racks next to it catch fire. After B ticks a burning GPU is DEAD (`dead` += 1) and a burning
    rack (after BR ticks) is a charred shell. S divides neither B nor BR, so no spread and burnout ever
    share a tick.
  * Le chaton fat is fat: after each step she needs a breather (she can walk at most every other tick;
    pressing a direction while out of breath only turns her). `splash` (directional, Space) throws one
    coolant: a warm / hot / burning GPU (or a cool one: it resets its timer) becomes cool again, a
    burning rack is saved. Bump into a CHILLER to refill to CAP coolant. `nap` (Z) waits one tick.
  * Win: the training run reaches 100 % (one Mistral logo per level; its variant sets the speed:
    1 % every 1 / 2 / 3 ticks). Lose: more GPUs dead than spare GPUs in the storeroom (`allow`).

Determinism: exec timers only (no Randomize / Probability / Triggers). A fire re-arms each spread
timer every S ticks whether the spread succeeded or not, so same-tick event order never matters.

Layout characters: W wall  c/w/h/f GPU cool/warm/hot/burning  R server rack (r burning)  C chiller  S spare GPU
  M/N/Q training run (1 % per 1/2/3 ticks)  x cable (floor decoration)  A le chaton fat  . floor
Validation per level: rectangular, one cat, one run; every GPU / rack / chiller has a reachable side;
the rollout planner (below) finds a winning run which is written to
apps/web/src/games/solutions/chatonfat.json; a do-nothing run must lose (the cat matters).
"""
from __future__ import annotations

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

GID = "chatonfat"
HERE = Path(__file__).resolve().parent
SOLUTIONS = C.ROOT / "studio/griddly-demo/apps/web/src/games/solutions" / f"{GID}.json"

TC, TW, TH = 60, 30, 20   # cool -> warm -> hot -> fire
S, B, BR = 4, 14, 6      # fire spread period, GPU burn time, rack burn time (racks are fuses)
CAP = 6                  # coolant carried
REST = 2                 # the cat walks at most once every REST ticks
RUN_PERIOD = {"M": 1, "N": 2, "Q": 3}
DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}

GPUS = ["gpu_cool", "gpu_warm", "gpu_hot"]
FIRES = ["gpu_fire", "rack_fire"]
SOLID = ["wall", "gpu_cool", "gpu_warm", "gpu_hot", "gpu_fire", "gpu_dead", "rack", "rack_fire", "rack_dead",
         "chiller", "spare", "run1", "run2", "run3"]
DECOR = ["cable"]
ALL = SOLID + DECOR + ["cat"]
CHARS = {"W": "wall", "c": "gpu_cool", "w": "gpu_warm", "h": "gpu_hot", "f": "gpu_fire", "R": "rack", "r": "rack_fire",
         "C": "chiller", "S": "spare", "M": "run1", "N": "run2", "Q": "run3", "x": "cable"}
HEAT_NEXT = {"gpu_cool": ("gpu_warm", TC), "gpu_warm": ("gpu_hot", TW), "gpu_hot": ("gpu_fire", TH)}


def lst(names):
    return "[" + ", ".join(names) + "]"


# ----------------------------------------------------------------------------------------------
# Art (all original, drawn by memekit helpers)
# ----------------------------------------------------------------------------------------------
def art():
    P = M.PALETTE
    load = lambda rel: C.load_png(C.IMAGES_DIR / rel)[2]
    flames = [load(p) for p in M.gpu_fire_frames()]
    hot = load(M.gpu("hot"))
    out = {}
    burning = []
    for i, fl in enumerate(flames):  # burning GPU: the hot card with flames licking up from it
        px = M.copy(hot)
        small = [[(0, 0, 0, 0)] * 24 for _ in range(24)]
        for y in range(24):
            for x in range(24):
                sx, sy = int(x / 0.8) - 3, int(y / 0.8) - 2
                if 0 <= sx < 24 and 0 <= sy < 24:
                    small[y][x] = fl[sy][sx]
        M.paste(px, small, 2, -3)
        burning.append(M.save(f"chatonfat-gpufire-{i}", px))
    out["gpu_fire"] = burning
    rack = load(M.server_rack("busy"))
    rf = []
    for i, fl in enumerate(flames):
        px = M.copy(rack)
        M.paste(px, fl, 0, 0)
        rf.append(M.save(f"chatonfat-rackfire-{i}", px))
    out["rack_fire"] = rf
    px = M.copy(load(M.server_rack("empty")))  # charred rack
    for y in range(24):
        for x in range(24):
            p = px[y][x]
            if p[3]:
                px[y][x] = (p[0] // 3 + 20, p[1] // 3 + 12, p[2] // 3 + 8, 255)
    out["rack_dead"] = M.save("chatonfat-rackdead", px)
    # chiller: a blue cooling unit with a big coolant tank on top
    px = M.blank()
    M.rect(px, 2, 9, 21, 21, P["steel"])
    M.rect(px, 2, 9, 21, 9, P["steel_hi"])
    M.rect(px, 21, 10, 21, 21, P["steel_dark"])
    for y in range(12, 20, 2):
        M.rect(px, 4, y, 19, y, P["steel_dark"])
    M.ellipse(px, 12, 6.5, 7.5, 5, P["coolant"], P["coolant_hi"], (30, 140, 180, 255))
    for d in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1)):
        M.put(px, 12 + d[0], 6 + d[1], P["white"])
    M.rect(px, 9, 16, 14, 17, P["led_blue"])
    M.outline(px)
    out["chiller"] = M.save("chatonfat-chiller", px)
    # spare GPU: a cardboard box with a GPU sticker
    px = M.blank()
    M.rect(px, 3, 7, 20, 20, (196, 150, 96, 255))
    M.rect(px, 3, 7, 20, 8, (220, 180, 120, 255))
    M.rect(px, 11, 7, 12, 20, (160, 118, 70, 255))
    M.rect(px, 5, 12, 9, 16, P["shroud"])
    M.rect(px, 5, 16, 9, 16, P["gold"])
    M.put(px, 7, 14, P["steel_hi"])
    M.rect(px, 14, 12, 18, 13, P["white"])
    M.rect(px, 14, 15, 17, 15, P["white"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22, rx=9)
    out["spare"] = M.save("chatonfat-spare", px)
    out["logo"] = M.mistral_logo_spin()
    out["cat"] = M.cat("fat")
    out["gpu"] = [M.gpu(s) for s in ("cool", "warm", "hot", "dead")]
    out["rack"] = M.rack_frames()
    out["cable"] = M.cable()
    out["floor"] = M.datacenter_floor()
    M.datacenter_wall16()
    return out


# ----------------------------------------------------------------------------------------------
# Levels
# ----------------------------------------------------------------------------------------------
LEVELS = [
    ("Night Shift", "Six GPUs warming up. Splash them cool (Space) and refill at the chiller. 100 ticks.", [
        "WWWWWWWWWWWWWWWWWWWW",
        "W..................W",
        "W.x...M......S.....W",
        "W.x................W",
        "W.xx...c.w.c.......W",
        "W...x..............W",
        "W....x.h.c.w...C...W",
        "W.....x............W",
        "W..A...xxxxxxxxx...W",
        "W..................W",
        "WWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Hot Aisle", "Twelve GPUs in two aisles, two already hot. 200 ticks, one spare.", [
        "WWWWWWWWWWWWWWWWWWWWWWWW",
        "W......................W",
        "W..N.............S.....W",
        "W......................W",
        "W...c.w.c...h.w.c......W",
        "W.........C............W",
        "W...w.c.w...h.c.w......W",
        "W......................W",
        "W..........A...........W",
        "WWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Rack & Ruin", "Server racks burn too and carry fire from GPU to GPU. A rack is already burning.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWW",
        "W.......................W",
        "W.N...........S.........W",
        "W.......................W",
        "W..rRRRRRRcRRwRRcRRRw...W",
        "W..........A............W",
        "W...c...w...C...c...w...W",
        "W.......................W",
        "W..RRRcRRRhRRRwRRRRcRR..W",
        "W.......................W",
        "W..x............x.......W",
        "WWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Two Halls", "Hall A and hall B, one chiller in the corridor between them. Juggle both for 200 ticks.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWWWW",
        "W........WWWWWWWWW........W",
        "W..c.c...W...N...W...c.w..W",
        "W........W.......W........W",
        "W..h.c...WW.....WW...c.c..W",
        "W..........x...x..........W",
        "W..c.w......xCx......c.c..W",
        "W..........x.A.x..........W",
        "W........WW.....WW........W",
        "W.S......W.......W......S.W",
        "WWWWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
    ("Le Grand Training Run", "The big one: 300 ticks, rack rows, two chillers, a fire already burning. Three spares.", [
        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
        "W............................W",
        "W.Q.....S.......S.......S....W",
        "W............................W",
        "W..fRRRRRcRRRRRRRRRRwRRRRRR..W",
        "W.......A....................W",
        "W...c...w.....C.....c...w....W",
        "W............................W",
        "W...w...c...........c...c....W",
        "W............................W",
        "W..RRRRRRwRRRRRRRRRRRcRRRR...W",
        "W............................W",
        "W.x.....................C..x.W",
        "W..xxxxxxxxxxxxxxxxxxxxxxxx..W",
        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    ]),
]


# ----------------------------------------------------------------------------------------------
# Simulator (mirrors the GDY)
# ----------------------------------------------------------------------------------------------
class Sim:
    __slots__ = ("w", "h", "mid", "oid", "pos", "coolant", "ready", "t", "events", "dead", "allow", "period",
                 "next_id", "result", "seq")

    def copy(self):
        s = Sim.__new__(Sim)
        s.w, s.h, s.mid, s.oid = self.w, self.h, list(self.mid), list(self.oid)
        s.pos, s.coolant, s.ready, s.t = self.pos, self.coolant, self.ready, self.t
        s.events = list(self.events)
        s.dead, s.allow, s.period, s.next_id, s.result, s.seq = (self.dead, self.allow, self.period, self.next_id,
                                                                 self.result, self.seq)
        return s

    def _create(self, c, name):
        """change_to / initial placement: a new object whose InitialActions start its timers."""
        self.next_id += 1
        self.mid[c], self.oid[c] = name, self.next_id
        if name in HEAT_NEXT:
            self._push(self.t + HEAT_NEXT[name][1], c, "heat", 0)
        elif name in FIRES:
            for d in DIRS:
                self._push(self.t + S, c, "spread", d)
            self._push(self.t + (B if name == "gpu_fire" else BR), c, "burnout", 0)

    def _push(self, t, c, kind, d):
        self.seq += 1
        self.events.append((t, self.seq, c, kind, d, self.oid[c]))

    def _run_events(self):
        due = [e for e in self.events if e[0] <= self.t]
        if not due:
            return
        self.events = [e for e in self.events if e[0] > self.t]
        due.sort()
        for t, _, c, kind, d, oid in due:
            if self.oid[c] != oid:
                continue  # source object was replaced / removed: the engine drops its delayed actions
            m = self.mid[c]
            if kind == "heat":
                self._create(c, HEAT_NEXT[m][0])
            elif kind == "burnout":
                if m == "gpu_fire":
                    self._create(c, "gpu_dead")
                    self.dead += 1
                else:
                    self._create(c, "rack_dead")
            else:
                self._push(self.t + S, c, "spread", d)
                dx, dy = DIRS[d]
                n = c + dx + dy * self.w
                tm = self.mid[n]
                if n != self.pos and tm in GPUS:
                    self._create(n, "gpu_fire")
                elif n != self.pos and tm == "rack":
                    self._create(n, "rack_fire")

    def walkable(self, c):
        return self.mid[c] is None or self.mid[c] in DECOR

    def step(self, action, d):
        """Player action ('move' 1-4, 'splash' 1-4, 'nap' 1), then one tick of timers."""
        if self.result:
            return self
        if action in ("move", "splash") and d in DIRS:
            dx, dy = DIRS[d]
            n = self.pos + dx + dy * self.w
            m = self.mid[n]
            if action == "move":
                if self.walkable(n) and self.t >= self.ready:
                    self.pos = n
                    self.ready = self.t + REST
                elif m == "chiller":
                    self.coolant = CAP
            elif self.coolant > 0:
                if m in GPUS or m == "gpu_fire":
                    self.coolant -= 1
                    self._create(n, "gpu_cool")
                elif m == "rack_fire":
                    self.coolant -= 1
                    self._create(n, "rack")
        self.t += 1
        self._run_events()
        if self.t >= 100 * self.period:
            self.result = "Win"
        elif self.dead > self.allow:
            self.result = "Lose"
        return self

    # ------------------------------------------------------------------ planner helpers
    def deadline(self, c, arrival=None):
        """Last tick at which splashing the object at c still saves it (a GPU: before it burns out; incoming
        fire through a fuse of racks / GPUs counts). For a burning rack: when its fire reaches a GPU."""
        m = self.mid[c]
        if arrival is None:
            arrival = self.fire_arrival()
        if m == "rack_fire":
            return min((arrival.get(n, 1 << 30) for n in self.downstream(c)), default=1 << 30)
        if m in FIRES:
            for t, _, cc, kind, _, oid in self.events:
                if cc == c and kind == "burnout" and oid == self.oid[c]:
                    return t - 1
            return self.t
        return min(self.ignite_time(c), arrival.get(c, 1 << 30)) + B - 1

    def downstream(self, c):
        """GPUs reachable from c through intact racks."""
        out, seen, q = [], {c}, [c]
        for x in q:
            for dx, dy in DIRS.values():
                n = x + dx + dy * self.w
                if n in seen:
                    continue
                seen.add(n)
                if self.mid[n] in GPUS:
                    out.append(n)
                elif self.mid[n] == "rack":
                    q.append(n)
        return out

    def fire_arrival(self):
        """Earliest tick fire can reach each flammable cell (Dijkstra; S ticks per hop, GPUs also burn on
        their own heat timers)."""
        import heapq
        dist, pq = {}, []
        for c, m in enumerate(self.mid):
            if m in FIRES:
                dist[c] = self.t
                pq.append((self.t, c))
            elif m in GPUS:
                dist[c] = self.ignite_time(c)
                pq.append((dist[c], c))
        heapq.heapify(pq)
        while pq:
            t, c = heapq.heappop(pq)
            if t > dist.get(c, 1 << 30):
                continue
            for dx, dy in DIRS.values():
                n = c + dx + dy * self.w
                if self.mid[n] in GPUS or self.mid[n] == "rack":
                    nt = t + S
                    if nt < dist.get(n, 1 << 30):
                        dist[n] = nt
                        heapq.heappush(pq, (nt, n))
        return dist

    def ignite_time(self, c):
        """Tick at which the object at c (a GPU) catches fire if left alone (fires: now)."""
        m = self.mid[c]
        if m in FIRES:
            return self.t
        for t, _, cc, kind, _, oid in self.events:
            if cc == c and kind == "heat" and oid == self.oid[c]:
                rest = {"gpu_cool": TW + TH, "gpu_warm": TH, "gpu_hot": 0}[m]
                return t + rest
        return 1 << 30


def load(rows) -> Sim:
    s = Sim.__new__(Sim)
    s.h, s.w = len(rows), len(rows[0])
    s.mid, s.oid = [None] * (s.w * s.h), [0] * (s.w * s.h)
    s.ready = s.t = s.dead = s.allow = s.next_id = s.seq = 0
    s.coolant = CAP
    s.events, s.result, s.period, s.pos = [], None, None, -1
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            c = y * s.w + x
            if ch == "A":
                s.pos = c
            elif ch in CHARS:
                s._create(c, CHARS[ch])
                if ch == "S":
                    s.allow += 1
                if ch in RUN_PERIOD:
                    s.period = RUN_PERIOD[ch]
    return s


# ----------------------------------------------------------------------------------------------
# Planner: macro actions ("go next to GPU g and splash it", "go refill") chosen by greedy rollouts
# ----------------------------------------------------------------------------------------------
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
    """Cells next to c the cat can stand on, mapped to the splash direction towards c."""
    out = {}
    for d, (dx, dy) in DIRS.items():
        n = c - dx - dy * s.w
        if s.walkable(n):
            out[n] = d
    return out


def idle_tick(s: Sim, log, target=None):
    """A tick the cat can't walk: splash a neighbouring GPU that is warm or worse (free, she is resting)
    if coolant allows (keeping one for the macro's target), else nap."""
    best, bd = None, None
    keep = 1 if target is not None and s.mid[target] not in ("chiller", None) else 0
    if s.coolant > keep:
        arr = s.fire_arrival()
        for d, (dx, dy) in DIRS.items():
            n = s.pos + dx + dy * s.w
            m = s.mid[n]
            if n != target and (m in FIRES or m in ("gpu_hot", "gpu_warm")):
                v = s.deadline(n, arr)
                if bd is None or v < bd:
                    best, bd = d, v
    if best is not None:
        s.step("splash", best)
        if log is not None:
            log.append(["splash", best])
    else:
        s.step("nap", 1)
        if log is not None:
            log.append(["nap", 1])


def run_macro(s: Sim, macro, log=None, limit=60):
    """Execute a macro on s in place. macro = ('splash', cell) | ('refill', chiller cell). Returns ok."""
    kind, target = macro
    goals = sides(s, target)
    if not goals:
        return False
    path, end = path_to(s, goals)
    if path is None:
        return False
    for d in path:
        while s.t < s.ready and not s.result:
            idle_tick(s, log, target)
        if s.result:
            return True
        s.step("move", d)
        if log is not None:
            log.append(["move", d])
    if s.result:
        return True
    act = "splash" if kind == "splash" else "move"
    if kind == "splash" and s.mid[target] not in GPUS + FIRES:
        return False
    s.step(act, goals[s.pos])
    if log is not None:
        log.append([act, goals[s.pos] if s.pos in goals else 0])
    return True


def candidates(s: Sim, k=5):
    """The k most urgent GPUs / burning racks (fires first), plus a refill when not full."""
    cand = []
    arr = s.fire_arrival()
    for c, m in enumerate(s.mid):
        if m in GPUS or m in FIRES:
            urgency = s.deadline(c, arr) - s.t
            cand.append((urgency, c))
    cand.sort()
    out = []
    if s.coolant > 0:
        out = [("splash", c) for _, c in cand[:k]]
    if s.coolant < CAP:
        chillers = [c for c, m in enumerate(s.mid) if m == "chiller"]
        dist = {}
        for ch in chillers:
            p, _ = path_to(s, sides(s, ch))
            if p is not None:
                dist[ch] = len(p)
        if dist:
            out.append(("refill", min(dist, key=dist.get)))
    return out, cand


def score(s: Sim):
    if s.result == "Win":
        return -1e9 + s.t
    if s.result == "Lose":
        return 1e8 - s.t * 1000
    fires = sum(1 for m in s.mid if m in FIRES)
    danger = 0
    arr = s.fire_arrival()
    for c, m in enumerate(s.mid):
        if m in GPUS:
            left = min(s.ignite_time(c), arr.get(c, 1 << 30)) - s.t
            danger += max(0, 30 - left) ** 2 / 30
    return s.dead * 10000 + fires * 400 + danger - s.coolant * 3


def greedy_macro(s: Sim, ws=0.35):
    """Cheapest worthwhile splash: travel time + a fraction of the slack; refill when dry."""
    cand, urg = candidates(s, k=12)
    if not cand:
        return None
    refill = next((m for m in cand if m[0] == "refill"), None)
    if s.coolant == 0:
        return refill
    best, bv = None, None
    arr = s.fire_arrival()
    for m in cand:
        if m[0] != "splash":
            continue
        p, _ = path_to(s, sides(s, m[1]))
        if p is None:
            continue
        travel = len(p) * REST
        slack = s.deadline(m[1], arr) - s.t
        if slack < travel + 1:
            continue  # can't make it in time
        if s.mid[m[1]] == "gpu_cool" and slack > 60:
            continue  # cooling a cool GPU early wastes coolant
        v = travel + ws * slack
        if bv is None or v < bv:
            best, bv = m, v
    if best is None:
        return refill
    return best


def rollout(s: Sim, horizon, ws=0.35):
    end = s.t + horizon
    while not s.result and s.t < end:
        m = greedy_macro(s, ws)
        if m is None or not run_macro(s, m):
            idle_tick(s, None)
    return score(s)


WS = (0.15, 0.35, 0.6, 1.0)
PLANS = [(h, w, None) for h, w in ((40, 6), (80, 8), (120, 10))]


def plan(rows, horizon=40, width=6, ws=0.35, max_ticks=400):
    s = load(rows)
    log = []
    while not s.result and s.t < max_ticks:
        cand, _ = candidates(s, k=width)
        for w2 in WS:
            g = greedy_macro(s, w2)
            if g and g not in cand:
                cand.append(g)
        best, bv = None, None
        for m in cand + [("nap", None)]:
            t = s.copy()
            if m[0] == "nap":
                idle_tick(t, None)
            elif not run_macro(t, m):
                continue
            v = min(rollout(t.copy(), horizon, w2) for w2 in WS)
            if bv is None or v < bv:
                best, bv = m, v
        if best is None or best[0] == "nap":
            idle_tick(s, log)
        else:
            run_macro(s, best, log)
    return s, log


# ----------------------------------------------------------------------------------------------
# GDY
# ----------------------------------------------------------------------------------------------
def actions():
    face = "{set_tile: meta.facing}"
    walk = lst(["_empty"] + DECOR)
    others = lst([n for n in ALL if n not in DECOR and n != "chiller"])
    splashable = lst(GPUS + ["gpu_fire"])
    not_splash = lst([n for n in ALL + ["_empty"] if n not in GPUS + FIRES])
    flammable_other = lst([n for n in ALL + ["_empty"] if n not in GPUS + ["rack"]])
    heat = "".join(f"      - Src: {{Object: {a}, Commands: [{{change_to: {b}}}]}}\n        Dst: {{Object: {a}}}\n"
                   for a, (b, _) in HEAT_NEXT.items())
    runs = "".join(f"      - Src: {{Object: run{p}, Commands: [{{incr: progress}}, {{exec: {{Action: train, ActionId: 1, "
                   f"Delay: {p}}}}}]}}\n        Dst: {{Object: run{p}}}\n" for p in (1, 2, 3))
    return f'''
Actions:
{C.face_action("cat")}
  # Heat: every GPU climbs cool -> warm -> hot -> fire on its own timer (started by InitialActions).
  - Name: heat
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Heat up, VectorToDest: [0, 0]}}
    Behaviours:
{heat}
  # Fire: each burning GPU / rack tries every neighbour every {S} ticks (one timer per direction,
  # always re-armed, so the order of same-tick events never matters).
  - Name: spread
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: {lst(FIRES)}, Commands: [{{exec: {{Action: spread, Delay: {S}}}}}]}}
        Dst: {{Object: {lst(GPUS)}, Commands: [{{change_to: gpu_fire}}]}}
      - Src: {{Object: {lst(FIRES)}, Commands: [{{exec: {{Action: spread, Delay: {S}}}}}]}}
        Dst: {{Object: rack, Commands: [{{change_to: rack_fire}}]}}
      - Src: {{Object: {lst(FIRES)}, Commands: [{{exec: {{Action: spread, Delay: {S}}}}}]}}
        Dst: {{Object: {flammable_other}}}

  # A fire burns out {B} ticks ({BR} for a rack) after it started: the GPU is dead, the rack a charred shell.
  - Name: burnout
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Burn out, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: gpu_fire, Commands: [{{incr: dead}}, {{change_to: gpu_dead}}]}}
        Dst: {{Object: gpu_fire}}
      - Src: {{Object: rack_fire, Commands: [{{change_to: rack_dead}}]}}
        Dst: {{Object: rack_fire}}

  # The training run: +1 % every 1 / 2 / 3 ticks depending on the logo variant.
  - Name: train
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Train, VectorToDest: [0, 0]}}
    Behaviours:
{runs}
  # Every spare GPU in the storeroom lets you lose one GPU.
  - Name: count_spare
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: spare, Commands: [{{incr: allow}}]}}
        Dst: {{Object: spare}}

  # Le chaton fat walks (at most every {REST} ticks: she is fat), bumps a chiller to refill coolant.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: cat
          Preconditions: [{{gte: [_steps, src.ready]}}]
          Commands: [{face}, {{mov: _dest}}, {{set: [ready, _steps]}}, {{add: [ready, {REST}]}}]
        Dst: {{Object: {walk}}}
      - Src:
          Object: cat
          Preconditions: [{{lt: [_steps, src.ready]}}]
          Commands: [{face}]
        Dst: {{Object: {walk}}}
      - Src: {{Object: cat, Commands: [{face}, {{set: [coolant, {CAP}]}}]}}
        Dst: {{Object: chiller}}
      - Src: {{Object: cat, Commands: [{face}]}}
        Dst: {{Object: {others}}}

  # Splash one coolant on the cell in the given direction.
  - Name: splash
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: cat
          Preconditions: [{{gt: [src.coolant, 0]}}]
          Commands: [{face}, {{decr: coolant}}, {{reward: 1}}]
        Dst: {{Object: {splashable}, Commands: [{{change_to: gpu_cool}}]}}
      - Src:
          Object: cat
          Preconditions: [{{gt: [src.coolant, 0]}}]
          Commands: [{face}, {{decr: coolant}}, {{reward: 1}}]
        Dst: {{Object: rack_fire, Commands: [{{change_to: rack}}]}}
      - Src: {{Object: cat, Commands: [{face}]}}
        Dst: {{Object: {not_splash}}}

  # Wait a tick (cats nap).
  - Name: nap
    InputMapping:
      Inputs:
        1: {{Description: Nap, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: cat, Commands: [{{nop: true}}]}}
        Dst: {{Object: cat}}
'''


def objects(a):
    fire_anim, fire_init = M.animate("gpu_fire", 3, 2)
    rfire_anim, rfire_init = M.animate("rack_fire", 3, 2)
    rack_anim, rack_init = M.animate("rack", 2, 4)
    logo_anims, logo_objs = "", []
    for p in (1, 2, 3):
        an, ini = M.animate(f"run{p}", 4, 3)
        logo_anims += an
        ch = {1: "M", 2: "N", 3: "Q"}[p]
        logo_objs.append(C.obj(f"run{p}", ch, 1, C.imgs(a["logo"]), "[1.0, 0.55, 0.0]", "square", 1.0,
                               ini + f"      - Action: train\n        ActionId: 1\n        Delay: {p}\n", tiles=4))
    heat = lambda name: f"    InitialActions:\n      - Action: heat\n        ActionId: 1\n        Delay: {HEAT_NEXT[name][1]}\n"
    spread = "".join(f"      - Action: spread\n        ActionId: {i}\n        Delay: {S}\n" for i in (1, 2, 3, 4))
    burn = spread + f"      - Action: burnout\n        ActionId: 1\n        Delay: {B}\n"
    rburn = spread + f"      - Action: burnout\n        ActionId: 1\n        Delay: {BR}\n"
    cat_extra = ("    Variables:\n      - Name: coolant\n        InitialValue: " + str(CAP) + "\n      - Name: ready\n"
                 "        InitialValue: 0\n" + C.FACE_INITIAL_ACTION)
    g = a["gpu"]
    objs = [
        C.obj("wall", "W", 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", "square", 1.0),
        C.obj("cable", "x", 0, C.img(a["cable"]), "[0.2, 0.4, 0.9]", "square", 0.4),
        C.obj("gpu_cool", "c", 1, C.img(g[0]), "[0.2, 0.8, 0.3]", "square", 0.8, heat("gpu_cool")),
        C.obj("gpu_warm", "w", 1, C.img(g[1]), "[0.9, 0.7, 0.2]", "square", 0.8, heat("gpu_warm")),
        C.obj("gpu_hot", "h", 1, C.img(g[2]), "[1.0, 0.3, 0.1]", "square", 0.8, heat("gpu_hot")),
        C.obj("gpu_fire", "f", 1, C.imgs(a["gpu_fire"]), "[1.0, 0.1, 0.0]", "circle", 0.9, fire_init + burn, tiles=3),
        C.obj("gpu_dead", None, 1, C.img(g[3]), "[0.3, 0.3, 0.3]", "square", 0.8),
        C.obj("rack", "R", 1, C.imgs(a["rack"]), "[0.25, 0.25, 0.3]", "square", 0.95, rack_init, tiles=2),
        C.obj("rack_fire", "r", 1, C.imgs(a["rack_fire"]), "[0.9, 0.2, 0.0]", "square", 0.95, rfire_init + rburn,
              tiles=3),
        C.obj("rack_dead", None, 1, C.img(a["rack_dead"]), "[0.15, 0.1, 0.1]", "square", 0.95),
        C.obj("chiller", "C", 1, C.img(a["chiller"]), "[0.2, 0.7, 0.9]", "square", 0.9),
        C.obj("spare", "S", 1, C.img(a["spare"]), "[0.8, 0.6, 0.4]", "square", 0.7,
              "    InitialActions:\n      - Action: count_spare\n        ActionId: 1\n"),
        *logo_objs,
        C.obj("cat", "A", 2, C.imgs(a["cat"]), "[1.0, 0.55, 0.15]", "triangle", 0.9, cat_extra, tiles=4),
    ]
    return "\nObjects:\n" + "".join(objs), fire_anim + rfire_anim + rack_anim + logo_anims


def header(a):
    variables = "".join(f"    - Name: {n}\n      InitialValue: 0\n" for n in ("progress", "dead", "allow"))
    return C.header("Le Chaton Fat", """
        Le chaton fat runs the datacenter. GPUs heat up over time (cool, warm, hot, ON FIRE) and fire
        spreads to neighbouring GPUs and server racks. Splash coolant on a GPU to cool it back down
        (or put out a fire); refill at a chiller. The cat is fat: she can only walk every other tick.
        Keep the GPUs alive until the training run reaches 100 %; lose if more GPUs die than you have
        spares.
        """.replace("        ", ""), "cat", ["gte: [progress, 100]"], ["gt: [dead, allow]"],
        background=a["floor"], variables=variables)


# ----------------------------------------------------------------------------------------------
# Validation / main / diff
# ----------------------------------------------------------------------------------------------
def validate(name, rows):
    C.check_rect(name, rows)
    assert sum(r.count("A") for r in rows) == 1, f"{name}: need one cat"
    assert sum(r.count(ch) for r in rows for ch in RUN_PERIOD) == 1, f"{name}: need one training run"
    for r in rows:
        assert set(r) <= set(CHARS) | {".", "A"}, f"{name}: unknown chars in {r}"
    a = C.find(rows, "A")[0]
    walk = lambda ch, p: ch in ".xA"
    reach = C.reach(rows, a, walk)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch in "cwhfrC":
                ok = any((x + dx, y + dy) in reach for dx, dy in DIRS.values())
                assert ok, f"{name}: {ch} at {(x, y)} has no reachable side"


def main():
    a = art()
    objs, anims = objects(a)
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
        for horizon, width, ws in PLANS:
            s, log = plan(rows, horizon, width, ws)
            if s.result == "Win" and (best is None or s.dead < best[0].dead):
                best = (s, log)
            if best and best[0].dead == 0:
                break
        assert best is not None, f"{name}: planner found no winning run"
        s, log = best
        # replay the log on a fresh sim (planner bookkeeping check)
        r = load(rows)
        for act, d in log:
            r.step(act, d)
        assert r.result == "Win" and r.t == len(log), f"{name}: replay mismatch {r.result} {r.t} {len(log)}"
        sols.append({"seed": 1, "actions": log})
        gpus = sum(r.count(ch) for r in rows for ch in "cwhf")
        print(f"  L{i} {name:22s} {len(rows[0])}x{len(rows)} gpus={gpus} run={100 * idle.period} ticks "
              f"spares={idle.allow}  idle: Lose at t={idle.t} ({idle.dead} dead)  planner: Win in {len(log)} "
              f"steps, {s.dead} dead ({time.time() - t0:.1f}s)", file=sys.stderr)
    gdy = "".join(parts) + actions() + anims + objs
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps({"format": 2, "levels": sols}))
    C.emit(GID, gdy, {
        "name": "Le Chaton Fat",
        "tagline": "The fat cat runs the datacenter. Keep the GPUs from catching fire.",
        "description": "GPUs heat up from cool to warm to hot to ON FIRE, and fire spreads to neighbouring "
                       "GPUs and server racks. Le chaton fat splashes coolant to cool them down and waddles "
                       "back to the chiller for more - but she is fat and walks only every other tick. Keep "
                       "the training run alive until 100 %.",
        "tags": ["meme", "action", "time-management", "fire"],
        "difficulty": "medium",
        "original": "",
        "accent": "#fb923c",
        "icon": "🐱",
        "controls": {"move": "move", "extra": [
            {"key": " ", "label": "Splash", "action": "splash", "directional": True},
            {"key": "z", "label": "Nap", "action": "nap", "directional": False}]},
        "hud": [{"label": "Training %", "kind": "global", "variable": "progress", "tone": "emerald"},
                {"label": "Coolant", "kind": "var", "object": "cat", "variable": "coolant", "tone": "sky"},
                {"label": "GPUs dead", "kind": "global", "variable": "dead", "tone": "rose"},
                {"label": "Spares", "kind": "global", "variable": "allow", "tone": "amber"},
                {"label": "Burning", "kind": "count", "object": "gpu_fire", "tone": "orange"}],
        "win_text": "Training run complete. Le chaton fat has earned a nap.",
        "lose_text": "Too many GPUs melted. The cat is not impressed.",
    })
    if "--diff" in sys.argv:
        diff()


def snapshot(s: Sim):
    objs = {}
    for c, m in enumerate(s.mid):
        if m and m != "wall":
            objs.setdefault(m, []).append([c % s.w, c // s.w])
    objs["cat"] = [[s.pos % s.w, s.pos // s.w]]
    for k in objs:
        objs[k].sort()
    return {"objs": objs, "vars": {"coolant": s.coolant, "ready": s.ready},
            "globals": {"dead": s.dead, "allow": s.allow, "progress": s.t // s.period}, "term": s.result}


def diff(n_random=5, length=200):
    rng = random.Random(7)
    sols = json.loads(SOLUTIONS.read_text())["levels"]
    jobs = []
    for i in range(len(LEVELS)):
        jobs.append({"level": i, "actions": sols[i]["actions"]})
        for _ in range(n_random):
            acts = []
            for _ in range(length):
                r = rng.random()
                acts.append(["move", rng.randint(1, 4)] if r < 0.6 else ["splash", rng.randint(1, 4)] if r < 0.9
                            else ["nap", 1])
            jobs.append({"level": i, "actions": acts})
    keep = [n for n in ALL if n != "wall"]
    for j in jobs:
        j["keep"], j["vars"] = keep, ["coolant", "ready"]
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
            if mine != eng:
                bad += 1
                keys = sorted(set(mine["objs"]) | set(eng["objs"]))
                delta = {k: (mine["objs"].get(k), eng["objs"].get(k)) for k in keys
                         if mine["objs"].get(k) != eng["objs"].get(k)}
                print(f"  DIFF level {job['level']} step {t} {job['actions'][t - 1] if t else None}: "
                      f"{mine['vars']}/{eng['vars']} {mine['globals']}/{eng['globals']} "
                      f"{mine['term']}/{eng['term']} {delta}", file=sys.stderr)
                break
    total = sum(len(s) for s in out)
    print(f"diff: {len(jobs)} runs, {total} states compared, {bad} mismatching runs", file=sys.stderr)
    if bad:
        sys.exit(1)


if __name__ == "__main__":
    main()
