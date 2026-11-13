#!/usr/bin/env python3
"""Bufo Party: a dance-floor clone puzzle (meme game, phase 4).

You are a bufo on the office dance floor. Every `party` bufo copies your moves: when you step in a
direction, all party bufos step the same way at the same time, each one blocked by walls, decor and any
bufo that can't move out of its way (a line of bufos walking into a wall stays put; bumping a wall
yourself still makes the others move). Sleepy bufos (zzz) don't dance until a dancing bufo bumps into
them - then they wake up and copy your moves from the next turn. Speakers can be pushed by any dancing
bufo. Win when EVERY bufo (you included) stands on a party pad at the same time; there are exactly as
many pads as bufos.

Engine rules (order-independent simultaneous moves):
  * `move` (player): sets globals cmd = direction, turn += 1 and execs `step` with the same direction.
  * `step`: every bufo has a guard `last != turn` and sets last = turn; walking into a dancing bufo
    first cascades `step` to it (it moves out of the way if it can), then moves if the cell is free.
  * party bufos run an internal `tick` every game tick (self-rescheduling, Delay 1) that execs `step`
    with ActionId = cmd. Because of the guard + cascade the result doesn't depend on the order in
    which the engine processes the bufos.
  * pads: `placed` (global) counts bufos on pads (each bufo's `onp`); `crowd` is counted at reset.
    Win: placed == crowd.  Lose: 600 steps.

Every level is solved by a BFS over (you, party bufos, sleepers, speakers) in an exact Python model
(Sim); the shortest solution is written to apps/web/src/games/solutions/bufoparty.json and replayed in
the engine; `--diff` compares Sim with the engine (office_dump.mjs) after every step.
Levels: hand-drawn rooms; bufos, pads and sleepers are placed by a seeded search that keeps the
placement with the longest shortest solution.

Map chars: w wall, Q rack, L logo, P plant (solid decor), A you, b party bufo, z sleepy bufo, o pad,
s speaker, ':' dance floor (walkable, animated), '.' office floor.
"""
from __future__ import annotations

import collections
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402
import office_kit as K  # noqa: E402

GID = "bufoparty"
DIRS = K.DIRS
SOLID = set("wQLP")
MAX_STEPS = 600


# ------------------------------------------------------------------------------ simulation
class Sim:
    """State: (avatar, party (sorted tuple), sleepers (frozenset), speakers (sorted tuple))."""

    def __init__(self, rows):
        self.rows = rows
        self.h, self.w = len(rows), len(rows[0])
        self.solid = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in SOLID}
        self.pads = frozenset(C.find(rows, "o"))
        self.start = (C.find(rows, "A")[0], tuple(sorted(C.find(rows, "b"))), frozenset(C.find(rows, "z")),
                      tuple(sorted(C.find(rows, "s"))))
        self.crowd = 1 + len(self.start[1]) + len(self.start[2])

    def step(self, state, d):
        av, party, sleep, spk = state
        dv = DIRS[d]
        movers = [av] + list(party)  # avatar first (player action), then the ticks
        pos = {p: p for p in movers}  # original position -> current position
        occupied = {p: p for p in movers}  # current cell -> original id
        spk = set(spk)
        sleep = set(sleep)
        woken = []
        done = set()

        def go(oid):
            if oid in done:
                return
            done.add(oid)
            cur = pos[oid]
            dst = (cur[0] + dv[0], cur[1] + dv[1])
            if dst in self.solid or not (0 <= dst[0] < self.w and 0 <= dst[1] < self.h):
                return
            if dst in sleep:
                sleep.discard(dst)
                woken.append(dst)
                return
            if dst in occupied:
                go(occupied[dst])
                if dst in occupied:
                    return
            if dst in spk:
                d2 = (dst[0] + dv[0], dst[1] + dv[1])
                if d2 in occupied:
                    go(occupied[d2])
                if (d2 in self.solid or d2 in spk or d2 in occupied or d2 in sleep or d2 in woken
                        or not (0 <= d2[0] < self.w and 0 <= d2[1] < self.h)):
                    return
                spk.discard(dst)
                spk.add(d2)
            if dst in woken:
                return
            del occupied[cur]
            occupied[dst] = oid
            pos[oid] = dst

        for oid in movers:
            go(oid)
        new_av = pos[av]
        new_party = tuple(sorted([pos[p] for p in party] + woken))
        return new_av, new_party, frozenset(sleep), tuple(sorted(spk))

    def won(self, state):
        av, party, sleep, _ = state
        return not sleep and av in self.pads and all(p in self.pads for p in party)

    def placed(self, state):
        av, party, _, _ = state
        return (av in self.pads) + sum(p in self.pads for p in party)


def bfs(sim, max_nodes=400_000):
    """A* over Sim states with h = max over bufos of the distance to the nearest pad (admissible:
    every bufo moves at most one cell per turn), so the result is a shortest solution."""
    import heapq
    pads = list(sim.pads)
    dist = {}

    def dpad(p):
        if p not in dist:
            dist[p] = min(abs(p[0] - q[0]) + abs(p[1] - q[1]) for q in pads)
        return dist[p]

    def h(s):
        return max([dpad(s[0])] + [dpad(p) for p in s[1]] + [dpad(p) for p in s[2]])

    start = sim.start
    par = {start: None}
    gcost = {start: 0}
    heap = [(h(start), 0, 0, start)]
    k = 0
    while heap:
        f, _, g, s = heapq.heappop(heap)
        if g != gcost[s]:
            continue
        if sim.won(s):
            out = []
            while par[s] is not None:
                s, d = par[s]
                out.append(d)
            return out[::-1], len(par)
        for d in DIRS:
            n = sim.step(s, d)
            if n not in gcost or gcost[n] > g + 1:
                gcost[n] = g + 1
                par[n] = (s, d)
                if len(par) > max_nodes:
                    return None, len(par)
                k += 1
                heapq.heappush(heap, (g + 1 + h(n), -g, g + 1, n) if False else (g + 1 + h(n), k, g + 1, n))
    return None, len(par)


# ------------------------------------------------------------------------------ levels
# Rooms: w walls, decor Q/L/P, ':' dance floor, '.' floor. Placement adds A/b/z/o/s on ':' or '.'.
ROOMS = {
    "lobby": """
wwwwwwwwwwwwwwwww
wQQ.....L.....PPw
w...:::::::::...w
w...:::::::::...w
wP..:::www:::..Pw
w...:::::::::...w
w...:::::::::...w
wQQ.....L.....QQw
wwwwwwwwwwwwwwwww
""",
    "kitchen": """
wwwwwwwwwwwwwwwwwww
wQQ......L......PPw
w..:::::::::::::..w
w..::w::::::::w:..w
w..:::::www::::::.w
wP.::::::::::::::Pw
w..:::w:::::w::::.w
w..:::::::::::::..w
wQQ.....PLP.....QQw
wwwwwwwwwwwwwwwwwww
""",
    "openspace": """
wwwwwwwwwwwwwwwwwwwww
wQQQ.....LLL.....PPPw
w...:::::::::::::...w
w...::::w::::w::::..w
wP..:::::::w:::::..Pw
w...::w:::::::::w:..w
w...:::::www::::::..w
wP..:::::::::::::..Pw
w...::::w:::::w:::..w
wQQQ.....LLL.....QQQw
wwwwwwwwwwwwwwwwwwwww
""",
    "rooftop": """
wwwwwwwwwwwwwwwwwwwwwww
wPP......QQQQQ......PPw
w..:::::::::::::::::..w
w..::::w:::::::w::::..w
w..::::::::w::::::::..w
wL.:::w:::::::::w::::Lw
w..::::::::w::::::::..w
w..::::w:::::::w::::..w
w..:::::::::::::::::..w
wPP......QQQQQ......PPw
wwwwwwwwwwwwwwwwwwwwwww
""",
    "afterparty": """
wwwwwwwwwwwwwwwwwwwwwwwww
wQQQ......LLLLL......QQQw
w....:::::::::::::::....w
w...::::w:::::::::w:::..w
wP..:::::::::w:::::::..Pw
w...:::ww:::::::::ww::..w
wL..:::::::::::::::::..Lw
w...:::ww:::::::::ww::..w
wP..:::::::::w:::::::..Pw
w...::::w:::::::::w:::..w
w....:::::::::::::::....w
wQQQ......LLLLL......QQQw
wwwwwwwwwwwwwwwwwwwwwwwww
""",
}

LEVELS = [
    dict(name="First Dance", room="lobby", party=1, sleep=0, spk=0, seed=11, tries=60,
         blurb="Your buddy copies every move. Use the walls to line you both up on the pads."),
    dict(name="Conga Line", room="kitchen", party=2, sleep=0, spk=0, seed=21, tries=60,
         blurb="Two party bufos copy you. Bump a wall to let them catch up."),
    dict(name="Wake-up Call", room="openspace", party=1, sleep=2, spk=0, seed=31, tries=60,
         blurb="Sleepy bufos (zzz) join the dance when a dancing bufo bumps into them."),
    dict(name="Bass Drop", room="rooftop", party=2, sleep=1, spk=1, seed=41, tries=60,
         blurb="Speakers can be pushed by any dancing bufo. Everyone onto a pad!"),
    dict(name="Afterparty", room="afterparty", party=2, sleep=2, spk=1, seed=51, tries=60,
         blurb="The whole office is dancing. Five bufos, five pads, one speaker."),
]


def room_rows(name):
    return [r for r in ROOMS[name].strip("\n").split("\n")]


def place(spec):
    """Constructive placement: put you, party bufos, sleepers and speakers at random, random-walk the
    dance (biased runs), and put the pads where every bufo ends up once all sleepers are awake. That is
    solvable by construction; BFS then finds the optimum (falls back to the walk if the BFS is capped).
    Keeps the candidate with the longest optimum."""
    base = room_rows(spec["room"])
    rng = random.Random(spec["seed"])
    floor = [(x, y) for y, r in enumerate(base) for x, c in enumerate(r) if c == ":"]
    edge = [(x, y) for y, r in enumerate(base) for x, c in enumerate(r) if c == "."]
    best = None
    stats = collections.Counter()
    for _ in range(spec["tries"]):
        g = [list(r) for r in base]
        av = rng.choice(edge)
        cells = rng.sample([c for c in floor if c != av], spec["party"] + spec["sleep"] + spec["spk"])
        g[av[1]][av[0]] = "A"
        it = iter(cells)
        for ch, n in (("b", spec["party"]), ("z", spec["sleep"]), ("s", spec["spk"])):
            for _ in range(n):
                x, y = next(it)
                g[y][x] = ch
        rows = ["".join(r) for r in g]
        sim = Sim(rows)
        s, walk = sim.start, []
        d = rng.choice(list(DIRS))
        for _ in range(rng.randint(40, 160)):
            if rng.random() < 0.35:
                d = rng.choice(list(DIRS))
            s = sim.step(s, d)
            walk.append(d)
        if s[2]:
            stats["asleep"] += 1
            continue  # somebody is still asleep
        ends = [s[0], *s[1]]
        starts = {sim.start[0], *sim.start[1], *sim.start[2], *sim.start[3]}
        if any(e in starts or base[e[1]][e[0]] not in ":." for e in ends):
            stats["ends"] += 1
            continue
        if sum(abs(e[0] - av[0]) + abs(e[1] - av[1]) for e in ends[:1]) < 4:
            continue
        g2 = [list(r) for r in rows]
        for x, y in ends:
            g2[y][x] = "o"
        rows2 = ["".join(r) for r in g2]
        sim2 = Sim(rows2)
        sol, nodes = bfs(sim2, max_nodes=spec.get("cap", 250_000))
        if sol is None:
            stats["cap"] += 1
            continue  # keep only A*-proven optima
        if len(sol) < spec.get("min", 8):
            continue
        if best is None or len(sol) > best[0]:
            best = (len(sol), rows2, sol, nodes)
    assert best, f"{spec['name']}: no solvable placement {dict(stats)}"
    return best


# ------------------------------------------------------------------------------ art
def pad_frames():
    out = []
    for f in range(3):
        px = M.blank((40, 34, 52, 255))
        cols = [M.P["mistral_yellow"], M.P["mistral_orange"], M.P["mistral_red"]]
        for y in range(24):
            for x in range(24):
                d = max(abs(x - 11.5), abs(y - 11.5))
                ring = int(d) // 3
                if d > 10.5:
                    px[y][x] = M.P["outline"]
                elif ring % 2 == 0:
                    px[y][x] = cols[(ring // 2 + f) % 3]
        # tiny star in the middle
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            px[12 + dy][12 + dx] = M.P["white"]
        out.append(M.save(f"bufoparty-pad-{f}", px))
    return out


def disco_frames():
    """Dance floor: 2x2 lit squares cycling through Mistral colours (3 frames)."""
    cols = [M.P["mistral_yellow"], M.P["mistral_amber"], M.P["mistral_orange"], M.P["mistral_redorange"],
            M.P["mistral_red"], (130, 70, 200, 255)]
    out = []
    for f in range(3):
        px = M.blank((34, 30, 44, 255))
        for qy in range(2):
            for qx in range(2):
                c = cols[(qx + 2 * qy + 2 * f) % len(cols)]
                dim = K.blend(c, (34, 30, 44), 0.55)
                for y in range(qy * 12 + 1, qy * 12 + 11):
                    for x in range(qx * 12 + 1, qx * 12 + 11):
                        px[y][x] = dim
                for x in range(qx * 12 + 2, qx * 12 + 5):
                    px[qy * 12 + 2][x] = K.blend(c, (255, 255, 255), 0.7)
        out.append(M.save(f"bufoparty-disco-{f}", px))
    return out


def sleepy_sprite():
    px = K.load(M.bufo_frames("plain")[1])  # blinking frame = eyes closed
    for x, y in ((17, 1), (18, 1), (19, 1), (18, 2), (17, 3), (18, 3), (19, 3),
                 (20, 5), (21, 5), (21, 6), (20, 7), (21, 7)):
        px[y][x] = (180, 200, 255, 255)
    return M.save("bufoparty-sleepy", px)


def speaker_sprite():
    px = M.blank()
    M.rect(px, 4, 2, 19, 21, (36, 34, 44, 255))
    M.rect(px, 4, 2, 19, 3, (70, 68, 84, 255))
    M.ellipse(px, 12, 7, 3.2, 3.0, (80, 80, 96, 255), (120, 120, 140, 255))
    M.ellipse(px, 12, 15, 5.5, 5.2, (80, 80, 96, 255), (120, 120, 140, 255))
    M.ellipse(px, 12, 15, 2.2, 2.0, M.P["mistral_orange"])
    M.put(px, 6, 4, M.P["led_green"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22.6, rx=8)
    return M.save("bufoparty-speaker", px)


# ------------------------------------------------------------------------------ GDY
BUFOS = ["bufo", "party", "woke"]
FLOOR = ["_empty", "disco"]
SOLID_OBJ = ["wall", "rack", "logo", "plant", "_boundary"]


def actions():
    floor = ", ".join(FLOOR)
    walk = ", ".join(BUFOS)
    guard = "Preconditions: [{neq: [src.last, turn]}]"
    on_pad = "{if: {Conditions: {eq: [onp, 0]}, OnTrue: [{incr: placed}, {set: [onp, 1]}]}}"
    off_pad = "{if: {Conditions: {eq: [onp, 1]}, OnTrue: [{decr: placed}, {set: [onp, 0]}]}}"
    beh, beh2 = [], []
    for b in BUFOS:
        face = "{set_tile: meta.facing}, " if b == "bufo" else ""
        pre = f"{{set: [last, turn]}}, {face}"
        beh += [
            f"      - Src: {{Object: {b}, {guard}, Commands: [{pre}{{mov: _dest}}, {off_pad}]}}\n"
            f"        Dst: {{Object: [{floor}]}}\n",
            f"      - Src: {{Object: {b}, {guard}, Commands: [{pre}{{mov: _dest}}, {on_pad}]}}\n"
            f"        Dst: {{Object: pad}}\n",
            f"      - Src: {{Object: {b}, {guard}, Commands: [{pre}{{exec: {{Action: step2}}}}]}}\n"
            f"        Dst: {{Object: [{walk}, speaker], Commands: [{{cascade: _dest}}]}}\n",
            f"      - Src: {{Object: {b}, {guard}, Commands: [{pre[:-2]}]}}\n"
            f"        Dst: {{Object: sleepy, Commands: [{{change_to: woke}}]}}\n",
            f"      - Src: {{Object: {b}, {guard}, Commands: [{pre[:-2]}]}}\n"
            f"        Dst: {{Object: [{', '.join(SOLID_OBJ)}]}}\n",
        ]
        beh2 += [
            f"      - Src: {{Object: {b}, Commands: [{{mov: _dest}}, {off_pad}]}}\n        Dst: {{Object: [{floor}]}}\n",
            f"      - Src: {{Object: {b}, Commands: [{{mov: _dest}}, {on_pad}]}}\n        Dst: {{Object: pad}}\n",
        ]
    beh += [f"      - Src: {{Object: speaker, Commands: [{{mov: _dest}}]}}\n        Dst: {{Object: [{floor}, pad]}}\n",
            f"      - Src: {{Object: speaker, Commands: [{{exec: {{Action: step2}}}}]}}\n"
            f"        Dst: {{Object: [{walk}], Commands: [{{cascade: _dest}}]}}\n"]
    beh2 += [f"      - Src: {{Object: speaker, Commands: [{{mov: _dest}}]}}\n        Dst: {{Object: [{floor}, pad]}}\n"]
    step_inputs = "\n".join(f"        {k}: {{VectorToDest: [{v[0]}, {v[1]}], MetaData: {{facing: {K.FACING[k]}}}}}"
                            for k, v in DIRS.items())
    move_inputs = "\n".join(f"        {k}: {{Description: {n}, VectorToDest: [{v[0]}, {v[1]}], MetaData: "
                            f"{{facing: {K.FACING[k]}, d: {k}}}}}"
                            for (k, v), n in zip(DIRS.items(), ("Left", "Up", "Right", "Down")))
    every = ", ".join(FLOOR[1:] + ["pad", "speaker", "sleepy"] + BUFOS + SOLID_OBJ + ["_empty"])
    return f"""
Actions:
{C.face_action("bufo")}  # You step; every party bufo copies the step this tick (see `step`).
  - Name: move
    InputMapping:
      Inputs:
{move_inputs}
    Behaviours:
      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {{set: [cmd, meta.d]}}, {{incr: turn}}, {{exec: {{Action: step, ActionId: meta.d}}}}]}}
        Dst: {{Object: [{every}]}}
  # One simultaneous dance step. A bufo walking into a dancing bufo first lets it move (cascade), so
  # lines of bufos move together; `last`/`turn` makes every bufo step at most once per turn.
  - Name: step
    InputMapping:
      Internal: true
      Inputs:
{step_inputs}
    Behaviours:
{"".join(beh)}  # Second half of a step into a cell that was occupied: after the occupant moved (or not), try
  # again against whatever is there now (exec without ActionId reuses the step's direction).
  - Name: step2
    InputMapping:
      Internal: true
    Behaviours:
{"".join(beh2)}  # Party bufos copy the last move every tick.
  - Name: tick
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: tick, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: [party, woke], Commands: [{{exec: {{Action: step, ActionId: cmd}}}}, {{exec: {{Action: tick, ActionId: 1, Delay: 1}}}}]}}
        Dst: {{Object: [party, woke]}}
  # A woken bufo starts dancing next turn.
  - Name: wake
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: wake, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: woke, Commands: [{{set: [last, turn]}}, {{exec: {{Action: tick, ActionId: 1, Delay: 1}}}}]}}
        Dst: {{Object: woke}}
  - Name: count_crowd
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: [bufo, party, sleepy], Commands: [{{incr: crowd}}]}}
        Dst: {{Object: [bufo, party, sleepy]}}
"""


def objects():
    acts, decor = K.decor_objects()
    pad_anim, pad_init = M.animate("pad", 3, 2, over=BUFOS + ["speaker"])
    disco_anim, disco_init = M.animate("disco", 3, 3, over=BUFOS + ["speaker", "sleepy"])
    party_anim, party_init = M.animate("party", 2, 3)
    woke_anim, woke_init = M.animate("woke", 2, 3)
    count = "      - Action: count_crowd\n        ActionId: 1\n"
    bvars = "    Variables:\n      - Name: last\n        InitialValue: 0\n      - Name: onp\n        InitialValue: 0\n"
    party_init_all = (party_init + "      - Action: tick\n        ActionId: 1\n        Delay: 1\n" + count)
    woke_init_all = woke_init + "      - Action: wake\n        ActionId: 1\n"
    idle = M.bufo_frames("party")
    out = [K.wall_object(), decor,
           C.obj("disco", ":", 0, C.imgs(disco_frames()), "[0.3, 0.2, 0.4]", tiles=3, extra=disco_init),
           C.obj("pad", "o", 0, C.imgs(pad_frames()), "[1.0, 0.7, 0.0]", "square", 0.8, tiles=3, extra=pad_init),
           C.obj("speaker", "s", 1, C.img(speaker_sprite()), "[0.2, 0.2, 0.25]", "square", 0.7),
           C.obj("sleepy", "z", 1, C.img(sleepy_sprite()), "[0.3, 0.5, 0.6]", "triangle", 0.6,
                 "    InitialActions:\n" + count),
           C.obj("party", "b", 1, C.imgs(idle), "[0.6, 0.9, 0.2]", "triangle", 0.7, bvars + party_init_all, tiles=2),
           C.obj("woke", None, 1, C.imgs(idle), "[0.6, 0.9, 0.2]", "triangle", 0.7, bvars + woke_init_all, tiles=2),
           C.obj("bufo", "A", 1, C.imgs(M.bufo("plain")), "[0.2, 0.9, 0.2]", "triangle", 0.9,
                 bvars + C.FACE_INITIAL_ACTION + count, tiles=4)]
    return acts + pad_anim + disco_anim + party_anim + woke_anim, "\nObjects:\n" + "".join(out)


# ------------------------------------------------------------------------------ main
def validate(name, rows):
    C.check_rect(name, rows)
    sim = Sim(rows)
    assert len(sim.pads) == sim.crowd, f"{name}: {len(sim.pads)} pads for {sim.crowd} bufos"
    sol, nodes = bfs(sim)
    assert sol, f"{name}: unsolvable"
    return sim, sol, nodes


def diff(gdy, runs, levels):
    here = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory() as td:
        gp, jp, op = Path(td, "g.yaml"), Path(td, "j.json"), Path(td, "o.json")
        gp.write_text(gdy)
        keep = BUFOS + ["sleepy", "speaker"]
        jp.write_text(json.dumps([{"level": levels.index(rows), "actions": [["move", d] for d in run], "keep": keep}
                                  for run, rows in zip(runs, levels)]))
        subprocess.run(["node", str(here / "office_dump.mjs"), str(gp), str(jp), str(op)], check=True,
                       stdout=subprocess.DEVNULL)
        dumps = json.loads(op.read_text())
    bad = 0
    for i, (run, states, rows) in enumerate(zip(runs, dumps, levels)):
        sim = Sim(rows)
        s = sim.start
        for k, st in enumerate(states):
            if k:
                s = sim.step(s, run[k - 1])
            o = st["objs"]
            eng = (next((x[1], x[2]) for x in o if x[0] == "bufo"),
                   tuple(sorted((x[1], x[2]) for x in o if x[0] in ("party", "woke"))),
                   frozenset((x[1], x[2]) for x in o if x[0] == "sleepy"),
                   tuple(sorted((x[1], x[2]) for x in o if x[0] == "speaker")))
            term = "Win" if sim.won(s) else None
            if eng != s or st["globals"]["placed"] != sim.placed(s) or st["term"] != term:
                print(f"  L{i} step {k}: sim={s} placed={sim.placed(s)} engine={eng} {st['globals']} {st['term']}",
                      file=sys.stderr)
                bad += 1
                break
        else:
            print(f"  L{i}: sim == engine over {len(states)} states", file=sys.stderr)
    return bad


def main():
    parts = [C.header("Bufo Party", """
        Bufo Party: every party bufo on the dance floor copies your moves - same direction, same time,
        each blocked by walls, decor and bufos that can't get out of the way. Sleepy bufos join the
        dance when a dancing bufo bumps into them; speakers can be pushed. Win when every bufo
        (you included) stands on a party pad at the same time.
        """.replace("        ", ""), "bufo", ["eq: [placed, crowd]"], [f"eq: [_steps, {MAX_STEPS}]"],
        background=K.floor_bg(), variables="    - Name: cmd\n      InitialValue: 0\n"
                                            "    - Name: turn\n      InitialValue: 0\n"
                                            "    - Name: placed\n      InitialValue: 0\n"
                                            "    - Name: crowd\n      InitialValue: 0\n")]
    runs, levels = [], []
    for i, spec in enumerate(LEVELS):
        score, rows, _, _ = place(spec)
        sim, sol, nodes = validate(spec["name"], rows)
        runs.append(sol)
        levels.append(rows)
        parts.append(C.level_block(spec["name"], spec["blurb"], rows))
        print(f"{i}. {spec['name']:13s} {len(rows[0])}x{len(rows)} bufos={sim.crowd} BFS optimum {len(sol)} moves "
              f"({nodes} states)", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    acts, objs = objects()
    gdy = "".join(parts) + actions() + acts + objs
    if "--diff" in sys.argv:
        rng = random.Random(7)
        fr, fl = list(runs), list(levels)
        for rows in levels:  # plus random dances (chains, wake-ups, speakers, wall bumps)
            for _ in range(6):
                fr.append([rng.choice([1, 2, 3, 4]) for _ in range(80)])
                fl.append(rows)
        sys.exit(1 if diff(gdy, fr, fl) else 0)
    K.write_solutions2(GID, [[("move", d) for d in run] for run in runs])
    C.emit(GID, gdy, {
        "name": "Bufo Party",
        "tagline": "Every bufo copies your moves. Get the whole party onto the pads.",
        "description": "A dance-floor clone puzzle: party bufos copy every step you take, all at once, and "
                       "walls stop them. Line everybody up on the party pads, wake the sleepy bufos and push "
                       "the speakers out of the way.",
        "tags": ["meme", "puzzle"],
        "difficulty": "medium",
        "original": "new (Mistral office meme)",
        "accent": "#a3e635",
        "icon": "🎉",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "On pads", "kind": "global", "variable": "placed", "tone": "lime"},
                {"label": "Bufos", "kind": "global", "variable": "crowd", "tone": "violet"},
                {"label": "Sleepy", "kind": "remaining", "object": "sleepy", "tone": "sky"}],
        "win_text": "Everybody on the pads - the party is on!",
        "lose_text": "The music stopped before everybody found a pad.",
    })


if __name__ == "__main__":
    main()
