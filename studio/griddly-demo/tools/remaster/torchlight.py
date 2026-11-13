#!/usr/bin/env python3
"""Torchlight: explore dark caverns with a torch that burns down, as seen over your shoulder.

An EGOCENTRIC game: the explorer turns and walks (1 turn left, 2 forward, 3 turn right, 4 back up
keeping your facing), and the default cameras rotate with them, so forward is always up.

Rules
  * The torch has `fuel` (global, 80). It burns 1 per tick, including when you wait or turn. At 0 you lose.
  * Torch sconces on the cave walls (`t`) and braziers on the floor (`b`) refuel the torch to full
    when you walk forward into them. That also lights them (+1, the first time). Lit sconces light a
    5x5 area and lit braziers a 7x7 area; spiders inside the light can't move (safe checkpoints).
  * Collect every crystal (`c`, +1 each) to win.
  * Bats (`v`) flee from your torch: within 3 tiles they fly directly away. A bat that is
    cornered next to you flaps at the flame (-1 fuel per tick). Walking into a bat scares it off for
    good, but its wings beat at the flame (-3 fuel). So bats never seal a tunnel.
  * Cave spiders (`x`) are weeping angels. They only creep towards you (1 step / 3 ticks, once
    within 6 tiles) while you are NOT looking at them. "Looking" means the spider is in the 90-degree
    cone ahead of you (forward distance 1..8, |sideways| <= forward). They also freeze in a lit
    brazier's light. A spider reaching you, or you backing into one, ends the run.
    Thrusting your torch at a spider right in front of you (forward into it) drives it back a tile
    and stuns it for 6 ticks.

How the AI is expressed in GDY (no Trigger/A* needed for the player logic): after every player move,
an internal `sync` action copies the avatar's x/y/facing (_x, _y, _dx, _dy) into globals ax/ay/adx/ady.
Spiders and bats run a self-scheduled think action. It computes their offset from the player, rotates
it into the player's frame (forward/sideways) with `if` blocks, and picks a direction id that it
passes to `exec` as `ActionId: <variable>`. A creep blocked by an obstacle sets a flag, and the
creature then tries the other axis. Only the light uses proximity Triggers (RANGE_BOX_AREA 2 / 3).

Layout characters (besides the shared terrain chars from common):
  w cave wall   A explorer (A[U/R/D/L] = initial facing)   c crystal   t torch sconce (on walls)
  b brazier   v bat   x spider   h shadow hunter   . floor

Shadow hunters (`h`, nightmare levels) lurk motionless while your torch burns bright. Once the fuel
drops to DARK or below they hunt you with A* pathfinding (exec + Search), one step every 2 ticks.
Thrusting the torch at one (forward into it) drives it back a tile and stuns it for 8 ticks; backing
into one, or letting it reach you, ends the run. Their logic only reads the global fuel and their own
variables (no Triggers), so it is deterministic.
"""
from __future__ import annotations

import collections
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import ego as E  # noqa: E402

GID = "torchlight"
S = C.SCENERY_CHARS
G = C.GROUND_CHARS
WATER, ROCK, STONE = S["water"], S["rock"], S["stone"]
BRIDGE = G["bridge"]
FUEL = 80
DARK = 40   # shadow hunters hunt while fuel <= DARK
WALK = set(".") | set(G.values())
DIRS4 = ((1, 0), (-1, 0), (0, 1), (0, -1))
TERRAIN = {"grass", "moss", "dirt", "sand", "bones", "mushroom", "skull", "bridge", "water", "rock", "stone"}


def grid(m):
    return ["".join(r) for r in m.g]


def ok(c, p):
    return c in WALK or c in "Acvxh"


def dist_map(rows, starts):
    d = {s: 0 for s in starts}
    q = collections.deque(starts)
    while q:
        x, y = q.popleft()
        for dx, dy in DIRS4:
            n = (x + dx, y + dy)
            if n not in d and 0 <= n[1] < len(rows) and 0 <= n[0] < len(rows[0]) and ok(rows[n[1]][n[0]], n):
                d[n] = d[(x, y)] + 1
                q.append(n)
    return d


def safe_set(m, a, p, ch):
    if m.get(*p) != "." or p == a:
        return False
    before = C.reach(grid(m), a, ok)
    m.set(*p, ch)
    if len(C.reach(grid(m), a, ok)) != len(before) - 1:
        m.set(*p, ".")
        return False
    return True


def connect4(m, chars, fill):
    for y in range(1, m.h - 2):
        for x in range(1, m.w - 2):
            for dx in (1, -1):
                a, b = m.get(x, y), m.get(x + dx, y + 1)
                if a in chars and b in chars and m.get(x + dx, y) not in chars and m.get(x, y + 1) not in chars:
                    m.set(x + dx, y, fill)


def cave(m, hubs, links, width=2, rad=(1.8, 2.8)):
    for i, j in links:
        m.walk(hubs[i], hubs[j], ".", width=width, wobble=0.3)
    for x, y in hubs:
        m.blob(x, y, m.rng.uniform(*rad), ".")
    connect4(m, set("."), ".")


def sconces(m, a, n, spacing=7):
    """Torch sconces on wall cells next to reachable floor, spread out (farthest-point sampling)."""
    rows = grid(m)
    r = C.reach(rows, a, ok)
    cand = [(x, y) for y in range(1, m.h - 1) for x in range(1, m.w - 1) if m.get(x, y) == "w"
            and any((x + dx, y + dy) in r and m.get(x + dx, y + dy) == "." for dx, dy in DIRS4)]
    placed = []
    m.rng.shuffle(cand)
    while len(placed) < n and cand:
        best = max(cand, key=lambda p: min([abs(p[0] - q[0]) + abs(p[1] - q[1]) for q in placed + [a]]))
        if placed and min(abs(best[0] - q[0]) + abs(best[1] - q[1]) for q in placed) < spacing:
            break
        placed.append(best)
        m.set(*best, "t")
        cand.remove(best)
    return placed


def place(m, a, ch, n, min_dist, region=None, clear=3):
    r = C.reach(grid(m), a, ok)
    spots = [p for p in m.free(region) if p in r and abs(p[0] - a[0]) + abs(p[1] - a[1]) >= min_dist
             and sum(m.get(p[0] + dx, p[1] + dy) in WALK for dx, dy in DIRS4) >= clear]
    m.rng.shuffle(spots)
    out = []
    for p in spots:
        if len(out) >= n:
            break
        if any(abs(p[0] - q[0]) + abs(p[1] - q[1]) < 4 for q in out):
            continue
        if ch in "b":
            if not safe_set(m, a, p, ch):
                continue
        else:
            m.set(*p, ch)
        out.append(p)
    return out


# ----------------------------------------------------------------------------- levels
def first_light():
    m = C.Map(27, 15, seed=301, fill="w", border="w")
    hubs = [(3, 7), (10, 3), (12, 11), (19, 6), (24, 10), (23, 3)]
    cave(m, hubs, [(0, 1), (0, 2), (1, 3), (2, 3), (3, 4), (3, 5)])
    a = (3, 7)
    m.set(*a, "A")
    sconces(m, a, 6, spacing=6)
    place(m, a, "b", 1, 10, (16, 2, 26, 13))
    place(m, a, "c", 5, 6)
    place(m, a, "v", 2, 8)
    place(m, a, "x", 1, 14, (18, 1, 26, 13))
    return m, a, "R", dict(moss=4, dirt=4, mush=4, bones=2)


def bat_roost():
    m = C.Map(31, 17, seed=302, fill="w", border="w")
    hubs = [(3, 3), (9, 9), (4, 14), (16, 4), (16, 13), (24, 8), (28, 3), (28, 14)]
    cave(m, hubs, [(0, 1), (1, 2), (1, 3), (1, 4), (3, 5), (4, 5), (5, 6), (5, 7), (3, 6)], width=1, rad=(1.6, 2.4))
    a = (3, 3)
    m.set(*a, "A")
    sconces(m, a, 8, spacing=6)
    place(m, a, "b", 1, 12)
    place(m, a, "c", 6, 6)
    place(m, a, "v", 6, 6, clear=2)
    place(m, a, "x", 1, 16)
    return m, a, "D", dict(dirt=6, bones=6, skull=3, mush=3)


def silk_galleries():
    m = C.Map(33, 19, seed=303, fill="w", border="w")
    hubs = [(3, 9), (10, 4), (10, 14), (17, 9), (24, 4), (24, 14), (30, 9)]
    cave(m, hubs, [(0, 1), (0, 2), (1, 3), (2, 3), (3, 4), (3, 5), (4, 6), (5, 6), (1, 4), (2, 5)])
    for _ in range(5):
        m.blob(m.rng.randint(4, 29), m.rng.randint(3, 16), 0.9, ROCK, only=".")
    a = (3, 9)
    m.set(*a, "A")
    for p in [(17, 9), (30, 9)]:
        safe_set(m, a, p, "b")
    sconces(m, a, 8, spacing=7)
    place(m, a, "c", 7, 7)
    place(m, a, "v", 3, 8)
    place(m, a, "x", 3, 12, clear=2)
    return m, a, "R", dict(moss=6, bones=10, skull=4, dirt=3)


def drowned_grotto():
    m = C.Map(33, 19, seed=304, fill="w", border="w")
    hubs = [(4, 4), (4, 15), (12, 9), (20, 4), (20, 15), (28, 9), (29, 3)]
    cave(m, hubs, [(0, 2), (1, 2), (2, 3), (2, 4), (3, 5), (4, 5), (5, 6), (0, 3)], width=2, rad=(2.2, 3.2))
    a = (4, 4)
    m.set(*a, "A")
    for cx, cy, r in [(12, 9, 1.6), (20, 15, 1.4), (24, 7, 1.2)]:
        cells = [(x, y) for y in range(int(cy - r - 1), int(cy + r + 2)) for x in range(int(cx - r - 1), int(cx + r + 2))
                 if ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5 <= r]
        for p in cells:
            safe_set(m, a, p, WATER)
    sconces(m, a, 9, spacing=7)
    place(m, a, "b", 2, 10)
    place(m, a, "c", 7, 7)
    place(m, a, "v", 4, 8)
    place(m, a, "x", 2, 12)
    return m, a, "D", dict(moss=10, sand=4, mush=6, bones=3)


def heart_of_the_mountain():
    m = C.Map(35, 21, seed=305, fill="w", border="w")
    hubs = [(17, 10), (5, 4), (5, 16), (12, 2), (23, 2), (29, 4), (29, 16), (22, 18), (11, 18)]
    cave(m, hubs, [(0, 1), (0, 2), (0, 5), (0, 6), (1, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 2)], width=2)
    a = (17, 10)
    m.set(*a, "A")
    for _ in range(6):
        m.blob(m.rng.randint(4, 30), m.rng.randint(3, 18), 0.9, ROCK, only=".")
    sconces(m, a, 10, spacing=7)
    place(m, a, "b", 2, 9)
    place(m, a, "c", 8, 8)
    place(m, a, "v", 5, 8)
    place(m, a, "x", 3, 12)
    return m, a, "U", dict(dirt=8, bones=10, skull=6, mush=4, moss=4)


def shadow_warrens():
    """Looping tunnels, few torches: three shadow hunters close in whenever the flame runs low."""
    m = C.Map(33, 19, seed=306, fill="w", border="w")
    hubs = [(3, 3), (12, 3), (22, 3), (29, 4), (6, 10), (16, 9), (26, 11), (3, 16), (14, 16), (29, 16)]
    cave(m, hubs, [(0, 1), (1, 2), (2, 3), (0, 4), (1, 5), (2, 5), (3, 6), (4, 5), (5, 6), (4, 7), (5, 8), (6, 9),
                   (7, 8), (8, 9)], width=1, rad=(1.4, 2.0))
    a = (3, 3)
    m.set(*a, "A")
    sconces(m, a, 6, spacing=9)
    place(m, a, "c", 7, 7)
    place(m, a, "v", 3, 8)
    place(m, a, "x", 2, 12)
    place(m, a, "h", 3, 14, clear=2)
    return m, a, "R", dict(dirt=6, bones=10, skull=6, mush=3)


def the_long_dark():
    """A deep, wide cave with a single brazier: four shadow hunters and two spiders."""
    m = C.Map(37, 21, seed=307, fill="w", border="w")
    hubs = [(18, 10), (5, 4), (5, 16), (13, 3), (24, 3), (32, 4), (32, 16), (24, 17), (12, 17)]
    cave(m, hubs, [(0, 1), (0, 2), (0, 5), (0, 6), (1, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 2), (1, 2)],
         width=2)
    for _ in range(6):
        m.blob(m.rng.randint(4, 32), m.rng.randint(3, 18), 0.9, ROCK, only=".")
    a = (18, 10)
    m.set(*a, "A")
    safe_set(m, a, (20, 10), "b")
    sconces(m, a, 6, spacing=10)
    place(m, a, "c", 8, 8)
    place(m, a, "v", 3, 8)
    place(m, a, "x", 2, 12)
    place(m, a, "h", 4, 14)
    return m, a, "L", dict(dirt=8, bones=12, skull=8, mush=4, moss=4)


LEVELS = [
    ("First Light", "Light the torches on the walls before your own flame dies.", first_light),
    ("Bat Roost", "Twisty tunnels full of skittish bats.", bat_roost),
    ("Silk Galleries", "Spiders only move when you look away. Keep them in your sight.", silk_galleries),
    ("Drowned Grotto", "Underground lakes and long stretches between torches.", drowned_grotto),
    ("Heart of the Mountain", "Every crystal in the heart of the mountain, and everything that guards them.",
     heart_of_the_mountain),
    ("Shadow Warrens", "☠ Nightmare: let your torch burn low and the shadow hunters come for you. Few torches, "
     "many tunnels.", shadow_warrens),
    ("The Long Dark", "☠ Nightmare: one brazier, six sconces and four shadow hunters in a deep cave.", the_long_dark),
]


# ----------------------------------------------------------------------------- validation
def validate(name, rows, a):
    C.check_rect(name, rows)
    assert len(C.find(rows, "A")) == 1, f"{name}: need exactly one explorer"
    cs = C.find(rows, "c")
    assert cs, f"{name}: no crystals"
    ref = C.find(rows, "t") + C.find(rows, "b")
    # standing spots in front of each refuel point
    spots = {}
    for p in ref:
        for dx, dy in DIRS4:
            q = (p[0] + dx, p[1] + dy)
            if 0 <= q[1] < len(rows) and 0 <= q[0] < len(rows[0]) and ok(rows[q[1]][q[0]], q):
                spots.setdefault(p, []).append(q)
    r = C.reach(rows, a, ok)
    assert all(c in r for c in cs), f"{name}: unreachable crystal"
    for x in C.find(rows, "x") + C.find(rows, "h"):
        assert abs(x[0] - a[0]) + abs(x[1] - a[1]) >= 10, f"{name}: spider too close to the start"
    # fuel budget (path length x1.4 for turns): the refuel network must connect the start to every
    # crystal: each crystal is a round trip from some refuel point.
    budget = FUEL - 6
    nodes = [a] + [q for p in spots for q in spots[p][:1]]
    dm = {n: dist_map(rows, [n]) for n in nodes}
    seen, q = {a}, collections.deque([a])
    while q:
        n = q.popleft()
        for k in nodes[1:]:
            if k not in seen and dm[n].get(k, 10 ** 9) * 1.4 <= budget:
                seen.add(k)
                q.append(k)
    worst = 0
    for c in cs:
        best = min(2 * dm[n].get(c, 10 ** 9) * 1.4 if n != a else dm[n].get(c, 10 ** 9) * 1.4 for n in seen)
        worst = max(worst, best)
        assert best <= budget, f"{name}: crystal {c} out of fuel range ({best:.0f} > {budget})"
    return len(cs), len(ref), len(seen) - 1, worst


# ----------------------------------------------------------------------------- GDY
SOLID = ["wall", "sconce", "sconce_lit", "brazier", "brazier_lit", "water", "rock", "stone"]
CRITTERS = ["bat", "spider", "hunter"]


def creep_action(name, mover, kill=False):
    """Absolute 1-step move (1 left, 2 up, 3 right, 4 down) for NPCs; sets `blocked` when it fails."""
    others = [o for o in SOLID + CRITTERS + ["crystal"] + (["avatar"] if not kill else [])]
    kill_rule = (f"      - Src: {{Object: {mover}, Commands: [{{rot: _dir}}]}}\n"
                 f"        Dst: {{Object: avatar, Commands: [{{remove: true}}]}}\n") if kill else ""
    return f'''  - Name: {name}
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0], OrientationVector: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1], OrientationVector: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0], OrientationVector: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1], OrientationVector: [0, 1]}}
        5: {{Description: Hover, VectorToDest: [0, 0]}}
        6: {{Description: Hover, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: {mover}, Commands: [{"{rot: _dir}, " if mover == "spider" else ""}{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
{kill_rule}      - Src: {{Object: {mover}, Commands: [{{set: [blocked, 1]}}]}}
        Dst: {{Object: [{", ".join(others)}]}}
'''


def rel_offset():
    """Commands computing rx, ry (creature - player), arx, ary (abs) and f, l (player frame)."""
    return '''            - set: [rx, _x]
            - sub: [rx, ax]
            - set: [ry, _y]
            - sub: [ry, ay]
            - set: [arx, rx]
            - if: {Conditions: {lt: [rx, 0]}, OnTrue: [{set: [arx, 0]}, {sub: [arx, rx]}]}
            - set: [ary, ry]
            - if: {Conditions: {lt: [ry, 0]}, OnTrue: [{set: [ary, 0]}, {sub: [ary, ry]}]}
'''


def actions():
    terrain_dst = C.EMPTY_OR_GROUND
    return f'''
Actions:
{E.sync_action("avatar")}
{E.wait_action("avatar")}
  # The torch burns 1 fuel every tick (self-scheduled).
  - Name: burn
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Burn, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: avatar, Commands: [{{decr: fuel}}, {{exec: {{Action: burn, ActionId: 1, Delay: 1}}}}]}}
        Dst: {{Object: avatar}}

  # Relative movement: 1 turn left, 2 forward, 3 turn right, 4 back up (facing kept).
  - Name: move
{E.REL_MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{rot: _dir}}, {E.SYNC}]}}
        Dst: {{Object: avatar}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {E.SYNC}]}}
        Dst: {{Object: {terrain_dst}}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{reward: 1}}, {E.SYNC}]}}
        Dst: {{Object: crystal, Commands: [{{remove: true}}]}}
      - Src:
          Object: avatar
          Preconditions: [{{eq: [meta.fwd, 1]}}]
          Commands: [{{set: [fuel, {FUEL}]}}, {{reward: 1}}]
        Dst: {{Object: [sconce, brazier], Commands: [{{change_to: _lit_}}]}}
      - Src:
          Object: avatar
          Preconditions: [{{eq: [meta.fwd, 1]}}]
          Commands: [{{set: [fuel, {FUEL}]}}]
        Dst: {{Object: [sconce_lit, brazier_lit]}}
      - Src: {{Object: avatar, Preconditions: [{{eq: [meta.fwd, 1]}}], Commands: [{{nop: []}}]}}
        Dst: {{Object: spider, Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: avatar, Preconditions: [{{eq: [meta.fwd, 0]}}], Commands: [{{remove: true}}]}}
        Dst: {{Object: spider}}
      - Src: {{Object: avatar, Preconditions: [{{eq: [meta.fwd, 1]}}], Commands: [{{nop: []}}]}}
        Dst: {{Object: hunter, Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: avatar, Preconditions: [{{eq: [meta.fwd, 0]}}], Commands: [{{remove: true}}]}}
        Dst: {{Object: hunter}}
      # (cascaded) a shadow hunter driven back by the torch, stunned for 8 ticks
      - Src: {{Object: hunter, Commands: [{{mov: _dest}}, {{set: [stun, 8]}}]}}
        Dst: {{Object: {terrain_dst}}}
      - Src: {{Object: hunter, Commands: [{{set: [stun, 8]}}]}}
        Dst: {{Object: [{", ".join(SOLID + CRITTERS + ["crystal"])}]}}
      # (cascaded) a spider driven back by the torch, stunned for 2 of its turns
      - Src: {{Object: spider, Commands: [{{mov: _dest}}, {{set: [stun, 2]}}, {{set_tile: 1}}]}}
        Dst: {{Object: {terrain_dst}}}
      - Src: {{Object: spider, Commands: [{{set: [stun, 2]}}, {{set_tile: 1}}]}}
        Dst: {{Object: [{", ".join(SOLID + CRITTERS + ["crystal"])}]}}
      - Src: {{Object: avatar, Commands: [{{sub: [fuel, 3]}}]}}
        Dst: {{Object: bat, Commands: [{{remove: true}}]}}

  # Light freezes spiders: lit sconces light a 5x5 area, lit braziers a 7x7 area.
  - Name: light
    Trigger: {{Type: RANGE_BOX_AREA, Range: 2}}
    Behaviours:
      - Src: {{Object: sconce_lit}}
        Dst: {{Object: spider, Commands: [{{set: [lit, 1]}}]}}
  - Name: bright_light
    Trigger: {{Type: RANGE_BOX_AREA, Range: 3}}
    Behaviours:
      - Src: {{Object: brazier_lit}}
        Dst: {{Object: spider, Commands: [{{set: [lit, 1]}}]}}

  # Spider (weeping angel): every 3 ticks, if awake (within 6) and not looked at / not lit, creep 1 step
  # towards the player along the longer axis, else the shorter one.
  - Name: stalk
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: spider
          Commands:
            - exec: {{Action: stalk, ActionId: 1, Delay: 3}}
{rel_offset()}            - if: {{Conditions: {{eq: [adx, 1]}}, OnTrue: [{{set: [f, rx]}}, {{set: [l, ry]}}]}}
            - if: {{Conditions: {{eq: [adx, -1]}}, OnTrue: [{{set: [f, 0]}}, {{sub: [f, rx]}}, {{set: [l, ry]}}]}}
            - if: {{Conditions: {{eq: [ady, 1]}}, OnTrue: [{{set: [f, ry]}}, {{set: [l, rx]}}]}}
            - if: {{Conditions: {{eq: [ady, -1]}}, OnTrue: [{{set: [f, 0]}}, {{sub: [f, ry]}}, {{set: [l, rx]}}]}}
            - set: [nl, 0]
            - sub: [nl, l]
            - set: [frozen, lit]
            - if: {{Conditions: {{gt: [stun, 0]}}, OnTrue: [{{decr: stun}}, {{set: [frozen, 1]}}]}}
            - if:
                Conditions: {{and: [{{gte: [f, 1]}}, {{lte: [f, 8]}}, {{lte: [l, f]}}, {{lte: [nl, f]}}]}}
                OnTrue: [{{set: [frozen, 1]}}]
            - set: [lit, 0]
            - if: {{Conditions: {{eq: [frozen, 1]}}, OnTrue: [{{set_tile: 1}}], OnFalse: [{{set_tile: 0}}]}}
            - set: [dir, 0]
            - set: [dir2, 0]
            - if:
                Conditions: {{and: [{{eq: [frozen, 0]}}, {{lte: [arx, 6]}}, {{lte: [ary, 6]}}]}}
                OnTrue:
                  - if: {{Conditions: {{gt: [rx, 0]}}, OnTrue: [{{set: [hx, 1]}}], OnFalse: [{{set: [hx, 3]}}]}}
                  - if: {{Conditions: {{gt: [ry, 0]}}, OnTrue: [{{set: [hy, 2]}}], OnFalse: [{{set: [hy, 4]}}]}}
                  - if: {{Conditions: {{eq: [rx, 0]}}, OnTrue: [{{set: [hx, 0]}}]}}
                  - if: {{Conditions: {{eq: [ry, 0]}}, OnTrue: [{{set: [hy, 0]}}]}}
                  - if: {{Conditions: {{gte: [arx, ary]}}, OnTrue: [{{set: [dir, hx]}}, {{set: [dir2, hy]}}], OnFalse: [{{set: [dir, hy]}}, {{set: [dir2, hx]}}]}}
            - set: [blocked, 0]
            - if: {{Conditions: {{gt: [dir, 0]}}, OnTrue: [{{exec: {{Action: creep, ActionId: dir}}}}]}}
            - if: {{Conditions: {{and: [{{eq: [blocked, 1]}}, {{gt: [dir2, 0]}}]}}, OnTrue: [{{exec: {{Action: creep, ActionId: dir2}}}}]}}
        Dst: {{Object: spider}}
{creep_action("creep", "spider", kill=True)}
  # Bat: every tick. Within 3 tiles of the torch it flies straight away (other axis if blocked); if it
  # is cornered right next to you it flaps at the flame (-1 fuel). Otherwise it flutters at random.
  - Name: flutter
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: bat
          Commands:
            - exec: {{Action: flutter, ActionId: 1, Delay: 1}}
{rel_offset()}            - set: [blocked, 0]
            - if:
                Conditions: {{and: [{{lte: [arx, 3]}}, {{lte: [ary, 3]}}]}}
                OnTrue:
                  - if: {{Conditions: {{gte: [rx, 0]}}, OnTrue: [{{set: [hx, 3]}}], OnFalse: [{{set: [hx, 1]}}]}}
                  - if: {{Conditions: {{gte: [ry, 0]}}, OnTrue: [{{set: [hy, 4]}}], OnFalse: [{{set: [hy, 2]}}]}}
                  - if: {{Conditions: {{gte: [arx, ary]}}, OnTrue: [{{set: [dir, hx]}}, {{set: [dir2, hy]}}], OnFalse: [{{set: [dir, hy]}}, {{set: [dir2, hx]}}]}}
                  - exec: {{Action: flap, ActionId: dir}}
                  - if: {{Conditions: {{eq: [blocked, 1]}}, OnTrue: [{{set: [blocked, 0]}}, {{exec: {{Action: flap, ActionId: dir2}}}}]}}
                  - if:
                      Conditions: {{and: [{{eq: [blocked, 1]}}, {{lte: [arx, 1]}}, {{lte: [ary, 1]}}]}}
                      OnTrue: [{{sub: [fuel, 1]}}]
                OnFalse:
                  - exec: {{Action: flap, Randomize: true}}
        Dst: {{Object: bat}}
{creep_action("flap", "bat")}
  # Shadow hunter: thinks every tick; when the torch is low (fuel <= {DARK}) and it is not stunned it
  # takes one A* step towards you every 2 ticks (Search: the avatar; everything solid is impassable).
  - Name: prowl
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Think, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: hunter
          Commands:
            - exec: {{Action: prowl, ActionId: 1, Delay: 1}}
            - if: {{Conditions: {{gt: [stun, 0]}}, OnTrue: [{{decr: stun}}]}}
            - set: [alert, 0]
            - if: {{Conditions: {{and: [{{lte: [fuel, {DARK}]}}, {{eq: [stun, 0]}}]}}, OnTrue: [{{set: [alert, 1]}}]}}
            - if: {{Conditions: {{eq: [alert, 1]}}, OnTrue: [{{set_tile: 1}}, {{incr: tick}}], OnFalse: [{{set_tile: 0}}, {{set: [tick, 0]}}]}}
            - if:
                Conditions: {{gte: [tick, 2]}}
                OnTrue:
                  - set: [tick, 0]
                  - exec: {{Action: hunt, Search: {{TargetObjectName: avatar, ImpassableObjects: [{", ".join(SOLID + ["bat", "spider", "hunter", "crystal"])}]}}}}
        Dst: {{Object: hunter}}
  - Name: hunt
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: hunter, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {terrain_dst}}}
      - Src: {{Object: hunter}}
        Dst: {{Object: avatar, Commands: [{{remove: true}}]}}
'''


def objects():
    # a single sprite, rotated by the renderer (RotateAvatarImage). New PNGs would not be in the
    # prebuilt WASM asset bundle, so only existing images are used.
    avatar = "oryx/oryx_fantasy/avatars/dwarf1.png"
    sp = "oryx/oryx_fantasy/avatars/spider1.png"
    vars_ = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: 0\n" for n in names)
    think = lambda act, delay: (f"    InitialActions:\n      - Action: {act}\n        ActionId: 1\n        Delay: {delay}\n")
    calc = ["rx", "ry", "arx", "ary", "hx", "hy", "dir", "dir2", "blocked"]
    av_extra = ("    InitialActions:\n      - Action: sync\n        ActionId: 1\n"
                "      - Action: burn\n        ActionId: 1\n        Delay: 1\n")
    return "\nObjects:\n" + C.terrain_objects(TERRAIN) + "".join([
        C.obj("wall", "w", 1, C.WALL_STYLES["dirt"], "[0.35, 0.25, 0.15]", "square", 1.0),
        C.obj("sconce", "t", 1, C.img("oryx/oryx_fantasy/stick-0.png"), "[0.5, 0.35, 0.2]", "triangle", 0.7),
        C.obj("sconce_lit", None, 1, C.img("gvgai/oryx/torch1.png"), "[1.0, 0.7, 0.1]", "triangle", 0.9),
        C.obj("brazier", "b", 1, C.img(C.TG + "barrel.png"), "[0.45, 0.3, 0.2]", "circle", 0.7),
        C.obj("brazier_lit", None, 1, C.img("gvgai/oryx/fire1.png"), "[1.0, 0.5, 0.0]", "circle", 1.0),
        C.obj("crystal", "c", 1, C.img("oryx/oryx_fantasy/ore-3.png"), "[0.3, 0.7, 1.0]", "circle", 0.6),
        C.obj("bat", "v", 2, C.img("gvgai/oryx/bat1.png"), "[0.6, 0.3, 0.7]", "triangle", 0.6,
              vars_(calc) + think("flutter", 1)),
        C.obj("spider", "x", 2, C.imgs([sp, "gvgai/oryx/spider3.png"]), "[0.9, 0.1, 0.1]", "triangle", 0.8,
              vars_(calc + ["f", "l", "nl", "lit", "frozen", "stun"]) + think("stalk", 2), tiles=2),
        C.obj("hunter", "h", 2, C.imgs(["gvgai/oryx/spider2.png", "gvgai/oryx/spider1.png"]), "[1.0, 0.5, 0.0]",
              "triangle", 0.8, vars_(["alert", "tick", "stun"]) + think("prowl", 1), tiles=2),
        C.obj("avatar", "A", 2, C.img(avatar), "[1.0, 0.8, 0.2]", "triangle", 0.8, av_extra),
    ])


def main():
    hdr = E.header("Torchlight", """
        Dark caverns seen over your shoulder (egocentric, rotating view). Your torch burns down every tick;
        walk into torch sconces and braziers to refuel. Collect every crystal. Bats flee your flame;
        cave spiders only move while you are not looking at them. In the deepest caves shadow hunters track
        you down whenever your torch burns low (fuel 40 or less).
        """.replace("        ", ""), "avatar", ["eq: [crystal:count, 0]"],
        ["eq: [avatar:count, 0]", "lte: [fuel, 0]"], background="oryx/oryx_fantasy/floor1-0.png",
        variables=f"    - Name: fuel\n      InitialValue: {FUEL}\n")
    parts = [hdr]
    for i, (name, blurb, fn) in enumerate(LEVELS):
        m, a, facing, recipe = fn()
        rows = m.rows(solid="wt" + ROCK + STONE)
        a2 = C.find(rows, "A")[0]
        nc, nr, nreach, worst = validate(name, rows, a2)
        ground = C.decorate(rows, recipe, seed=7300 + i, stack_under="Acvxh")
        parts.append(E.level_block(name, blurb, rows, ground, {a2: facing}))
        print(f"{i}. {name:22s} {len(rows[0])}x{len(rows)} crystals={nc} refuel={nr} (reachable {nreach}) "
              f"worst-trip={worst:.0f}/{FUEL} spiders={len(C.find(rows, 'x'))} bats={len(C.find(rows, 'v'))} "
              f"hunters={len(C.find(rows, 'h'))}",
              file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    gdy = gdy.replace("change_to: _lit_", "change_to: _LIT_")
    # sconce -> sconce_lit, brazier -> brazier_lit (one rule each)
    gdy = gdy.replace('''      - Src:
          Object: avatar
          Preconditions: [{eq: [meta.fwd, 1]}]
          Commands: [{set: [fuel, %d]}, {reward: 1}]
        Dst: {Object: [sconce, brazier], Commands: [{change_to: _LIT_}]}
''' % FUEL, "".join(f'''      - Src:
          Object: avatar
          Preconditions: [{{eq: [meta.fwd, 1]}}]
          Commands: [{{set: [fuel, {FUEL}]}}, {{reward: 1}}]
        Dst: {{Object: {o}, Commands: [{{change_to: {o}_lit}}]}}
''' for o in ("sconce", "brazier")))
    C.emit(GID, gdy, {
        "name": "Torchlight",
        "tagline": "Your torch is burning down. Don't look away from the spiders.",
        "description": "Explore pitch-dark caverns from a first-person, rotating view. Relight wall sconces "
                       "and braziers to keep your torch alive while you collect every crystal. Bats flee "
                       "your flame, cave spiders only creep closer while you aren't looking at them, and in the deepest caves "
                       "shadow hunters track you down whenever your torch burns low.",
        "tags": ["exploration", "egocentric", "survival", "partial-observability"],
        "difficulty": "hard",
        "original": "",
        "accent": "#f59e0b",
        "icon": "🔥",
        "controls": {"move": "move", "scheme": "relative",
                     "extra": [{"key": " ", "label": "Wait", "action": "wait"}]},
        "hud": [
            {"label": "Fuel", "kind": "global", "variable": "fuel", "tone": "orange"},
            {"label": "Crystals", "kind": "collected", "object": "crystal", "tone": "sky"},
            {"label": "Torches lit", "kind": "count", "object": "sconce_lit", "tone": "amber"},
            {"label": "Spiders", "kind": "count", "object": "spider", "tone": "rose"},
            {"label": "Hunters", "kind": "count", "object": "hunter", "tone": "violet"},
        ],
        "win_text": "Every crystal gathered. You climb back into the daylight!",
        "lose_text": "Darkness takes you.",
    })


if __name__ == "__main__":
    main()
