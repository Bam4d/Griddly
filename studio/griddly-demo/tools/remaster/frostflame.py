#!/usr/bin/env python3
"""Frost & Flame: an elemental push puzzle on a frozen mountain (new game, not a port).

Rules (the GDY below; frostflame_sim.py mirrors them exactly for the solver):
  * ICE floor is slippery: the avatar, ice blocks and braziers that move onto ice keep sliding in the
    same direction until something stops them (they stop *before* a wall/thing/open water, or step
    off the ice onto snow and stop there).
  * Elements meet: ICE BLOCK + WATER -> ice (the block sinks and freezes it: a slippery bridge);
    BRAZIER + ICE -> water (a brazier pushed onto ice melts through that tile and goes out);
    BRAZIER + WATER -> the fire goes out; BRAZIER + beacon SOCKET -> the beacon is lit.
  * Heat: every brazier and lit beacon melts snowmen, frost walls and ice blocks in the 8 cells around
    it at the end of each move (the avatar carries a lantern: it melts nothing).
  * Goal: light every beacon socket and reach the shrine (whichever the level has). You lose when there
    are fewer braziers left than unlit sockets.

Implementation notes:
  * Sliding = exec chains: a move onto ice runs the internal `slide` action (same direction, no
    ActionId -> the engine reuses the triggering action's vector), which re-execs itself while the
    mover keeps landing on ice. Ice blocks carry `slip` (1 = standing on ice) so the pusher, which
    steps into the block's old cell, knows whether to keep sliding (copied to its `pend` var before
    the cascade). Braziers never stand on ice, so they need no slip.
  * Heat is a proximity trigger (RANGE_BOX_BOUNDARY, range 1) from braziers/beacons; it only removes
    non-source objects (snowmen, frost walls, blocks).
  * Everything that blocks or is pushed is Z=1 (pusher shares the pushee's Z, see shared notes); ice,
    open water and ground decoration are Z=0.

Layout characters (besides the shared terrain chars from common):
  w icy rock wall  A avatar  i ice  p open water  b ice block  f brazier  s snowman  z frost wall
  x beacon socket  g shrine  . snow
Validation: every level is solved by a BFS over (avatar region, blocks, braziers, ice, water, snowmen,
sockets, shrine) - frostflame_sim.py - and the solution is written to apps/web/src/games/solutions/
frostflame.json (replay_solutions.mjs replays it in the engine). Ablations prove each level needs its
mechanics (e.g. unsolvable when ice can't be entered / blocks can't freeze water / fire can't melt).
`--diff` cross-checks the simulator against the engine on random walks of every level.
"""
from __future__ import annotations

import collections
import json
import random
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import frostflame_sim as S  # noqa: E402
import pushlib as P  # noqa: E402

GID = "frostflame"
HERE = Path(__file__).resolve().parent
SC = C.SCENERY_CHARS
SOLID_TERRAIN = {SC["rock"], SC["stone"], SC["tree"], SC["bush"], SC["tombstone"]}
WALL_CHARS = "w" + "".join(SOLID_TERRAIN)
TERRAIN_USED = {"rock", "stone", "tree", "bush", "tombstone", "grass", "dirt", "moss", "bones", "skull", "sand"}
GROUND_NAMES = [n for n in C.GROUND_NAMES if n in TERRAIN_USED]
SCENERY = [n for n in C.SCENERY_NAMES if n in TERRAIN_USED]
FLOOR = "[_empty, " + ", ".join(GROUND_NAMES) + "]"
BLOCKERS = "[wall, snowman, frostwall, socket, beacon, pond, " + ", ".join(SCENERY) + "]"

# ----------------------------------------------------------------------------------------------
# Sprites (generated into resources/images/remastered/frostflame-*.png)
# ----------------------------------------------------------------------------------------------
IMG = "remastered/frostflame-"


def _rng_px(seed):
    return random.Random(seed)


def make_sprites():
    d = C.IMAGES_DIR
    rng = _rng_px(7)
    # snow background: soft white-blue with sparkles
    px = [[(226 + rng.randint(-6, 4), 234 + rng.randint(-5, 4), 243 + rng.randint(-4, 3), 255) for _ in range(24)]
          for _ in range(24)]
    for _ in range(10):
        x, y = rng.randrange(24), rng.randrange(24)
        px[y][x] = (205, 215, 232, 255)
    for _ in range(5):
        x, y = rng.randrange(24), rng.randrange(24)
        px[y][x] = (255, 255, 255, 255)
    C.save_png(d / f"{IMG}snow.png", 24, 24, px)
    # ice: glossy pale cyan with diagonal glints and a faint rim
    rng = _rng_px(11)
    px = []
    for y in range(24):
        row = []
        for x in range(24):
            r, g, b = 150 + rng.randint(-4, 4), 214 + rng.randint(-4, 4), 236 + rng.randint(-3, 3)
            if (x + y) % 11 in (0, 1) and 3 < x < 21 and 3 < y < 21:
                r, g, b = 205, 242, 252
            if x in (0, 23) or y in (0, 23):
                r, g, b = 128, 192, 222
            row.append((r, g, b, 255))
        px.append(row)
    C.save_png(d / f"{IMG}ice.png", 24, 24, px)
    # ice block: translucent-looking cube with bevel
    px = [[(0, 0, 0, 0)] * 24 for _ in range(24)]
    for y in range(2, 22):
        for x in range(2, 22):
            edge = min(x - 2, y - 2, 21 - x, 21 - y)
            if edge == 0:
                c = (60, 120, 170, 255)
            elif edge <= 2:
                c = (200, 238, 252, 255) if (x - 2 <= 2 or y - 2 <= 2) else (110, 175, 215, 255)
            else:
                c = (160, 215, 240, 255)
                if (x - y) in (0, 1) and 6 < x < 18:
                    c = (230, 250, 255, 255)
            px[y][x] = c
    C.save_png(d / f"{IMG}block.png", 24, 24, px)
    fire_w, fire_h, fire = C.load_png(d / "gvgai/oryx/fire1.png")

    def bowl(base_px, top=15, col=(70, 62, 60, 255), rim=(120, 108, 100, 255)):
        for y in range(top, 23):
            half = 9 - (y - top)
            for x in range(12 - half, 12 + half):
                base_px[y][x] = rim if y == top else col
        return base_px

    def overlay(dst, src, scale=0.75, oy=-3):
        h, w = len(src), len(src[0])
        for y in range(24):
            for x in range(24):
                sx = int((x - 12) / scale + w / 2)
                sy = int((y - 12 - oy) / scale + h / 2)
                if 0 <= sx < w and 0 <= sy < h and src[sy][sx][3] > 0:
                    dst[y][x] = src[sy][sx]
        return dst

    braz = [[(0, 0, 0, 0)] * 24 for _ in range(24)]
    braz = overlay(braz, fire, 0.75, -2)
    braz = bowl(braz)
    C.save_png(d / f"{IMG}brazier.png", 24, 24, braz)
    # socket (unlit beacon): stone pedestal with an empty dark bowl; lit beacon: pedestal + flame
    def pedestal(lit):
        p = [[(0, 0, 0, 0)] * 24 for _ in range(24)]
        for y in range(12, 23):
            for x in range(5, 19):
                p[y][x] = (98, 104, 118, 255) if x not in (5, 18) and y != 22 else (60, 64, 76, 255)
        for x in range(3, 21):
            p[11][x] = (150, 156, 170, 255)
            p[12][x] = (60, 64, 76, 255)
        for y in (15, 18):
            for x in range(6, 18):
                p[y][x] = (80, 86, 100, 255)
        if lit:
            p = overlay(p, fire, 0.6, -5)
            for x in range(3, 21):
                p[11][x] = (240, 190, 90, 255)
        else:
            for x in range(6, 18):
                p[10][x] = (40, 40, 48, 255)
            for x in range(8, 16):
                p[9][x] = (40, 40, 48, 255)
        return p
    C.save_png(d / f"{IMG}socket.png", 24, 24, pedestal(False))
    C.save_png(d / f"{IMG}beacon.png", 24, 24, pedestal(True))


# ----------------------------------------------------------------------------------------------
# Levels
# ----------------------------------------------------------------------------------------------
LEVELS = []  # filled below: dict(name, blurb, rows, ground recipe, ablations)


def level(name, blurb, art, recipe=None, needs=(), seed=0):
    rows = [r for r in art.strip("\n").splitlines()]
    w = max(len(r) for r in rows)
    rows = [list(r.ljust(w, "w")) for r in rows]
    # cosmetic: solid scenery buried in solid mass becomes icy rock wall, trees stay on the fringe
    # (all solid chars are equivalent to the rules/solver)
    h = len(rows)
    buried = [(x, y) for y in range(1, h - 1) for x in range(1, w - 1) if rows[y][x] in SOLID_TERRAIN
              and all(rows[y + dy][x + dx] in WALL_CHARS for dx, dy in S.NB8)]
    for x, y in buried:
        rows[y][x] = "w"
    rows = ["".join(r) for r in rows]
    LEVELS.append(dict(name=name, blurb=blurb, rows=rows, recipe=recipe or {}, needs=needs, seed=seed))


LEVEL_SPECS = [
    dict(name="Glacier Rink", blurb="Ice is slippery: you slide until something stops you. Reach the shrine.",
         needs=("noice",), recipe=dict(sand=2, skull=1), art="""
wwwwwwwwwwwwwwwwwwwwww
w..&iiii@iiiiii@iiiiiw
w..&iiiiiiiiii@iiiiiiw
w..&iiii@@iiiiiiiiiiiw
w..&iiiiiiiiiiiiiiiiiw
w..&iiiiii@iiiiiiiii#w
wA.iiii.iiii.i.ii#igiw
w..&i#iiiii@ii&iiiiiiw
w..&iii@iiiiiiiiiiiiiw
w..&iii#ii&iiiiiiiiiiw
w..&iiiiii&iiiiiiiiiiw
w..&ii#iiiiiiiiiii@@@w
wwwwwwwwwwwwwwwwwwwwww
"""),
    dict(name="Frozen Ford", blurb="Push ice blocks into the river: each one freezes a tile into an ice bridge.",
         needs=("freeze", "noice"), recipe=dict(sand=3, dirt=1, bones=1), art="""
wwwwwwwwwwwwwwwwwwwwwwww
w&&&&&&&&&&ppp&&&&&&&&&w
w&&&&..&&&&ppp&&&&&&&&&w
w&&&....&&&ppp&&&...&&&w
w&&..b.....ppp.......&&w
w&A..b..b..ppp....g...&w
w&&........ppp.......&&w
w&&&....&&&ppp&&&...&&&w
w&&&&..&&&&ppp&&&&&&&&&w
w&&&&&&&&&&ppp&&&&&&&&&w
wwwwwwwwwwwwwwwwwwwwwwww
"""),
    dict(name="Snowmen's Pass", blurb="Braziers melt snowmen next to them. Light both beacons, then visit the shrine.",
         needs=("heat", "push_brazier"), recipe=dict(sand=3, dirt=2, bones=2), art="""
wwwwwwwwwwwwwwwwwwwwwwww
w&&&&&&&wwwwwwwww&&&&&&w
w&......w...x...w.....&w
w&..f...w.......w..x..&w
w&......ws.sss.sw.....&w
w&..A.........s.......&w
w&......ws....sswsssssw
w&&..f..w.......w.....&w
w&&.....wppp....w..g..&w
w&&&&&&&wwwwwwwww&&&&&&w
wwwwwwwwwwwwwwwwwwwwwwww
"""),
    dict(name="Thin Ice", blurb="A brazier pushed onto ice melts a hole and goes out. Use holes to stop your slide.",
         needs=("push_brazier", "noice"), recipe=dict(sand=2, skull=1), art="""
wwwwwwwwwwwwwwwwwwwwwwwwww
w&&&&&&&&&&&&&&&&&&&&&&&&w
w&....iiiiiiiiiiiiiiiiii&w
w&....iiiiiiiiiiiiiiii@i&w
w&....iiiiiii#i@iiiiiiii&w
w&....iiiiiiiiiiiiiiiiii&w
w&....iiiiiiiiiiiii..iii&w
w&.A..iiiiiiiiiiiii.fiii&w
w&....iiiiiiiif.iiiiiiii&w
w&....iiiiiiii..iiiiii@i&w
w&....iiiiiiiiiiiiiiiiii&w
w&&&&&&&&&&&&&&&&g&&&&&&&w
wwwwwwwwwwwwwwwwwwwwwwwwww
"""),
    dict(name="Bridge of Fire", blurb="Freeze a way across, carry the fire over and light both beacons.",
         needs=("freeze", "heat", "push_brazier"), recipe=dict(sand=3, dirt=2, bones=1), art="""
wwwwwwwwwwwwwwwwwwwwwwww
w&&&&&&&&ppp&&&&&&&&&&&w
w&.......ppp.....s...x&w
w&.A..b..ppp..f..s....&w
w&.......ppp.....sssss&w
w&..b....ppp..........&w
w&.......ppp....f.....&w
w&..b..&&ppp&&....zzz.&w
w&.......ppp...x..zgz.&w
w&&&&&&&&ppp&&&&&&&&&&w
wwwwwwwwwwwwwwwwwwwwwww
"""),
    dict(name="Summit Shrine", blurb="Everything at once: slide, freeze, melt and burn your way to the summit.",
         needs=("freeze", "heat", "noice", "push_brazier"), recipe=dict(sand=4, dirt=2, bones=2, skull=1), art="""
wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
w&&&&&&&&&&&&&&&&&&&&&&&&&&&&w
w&.....&iiiiiiiiii@@ip......&w
w&.....&iiiiiiiiiiiiip......&w
w&.b...iii&iiiiiiiiiip..x...&w
w&..A..&iiiii@iiiiiiip......&w
w&.....&iiiiiiiiiiiiipssssss&w
w&.....iiiiiiiiiiiiiip......&w
w&.....iiiii@iiiiiiiip......&w
w&.b...&iiiiiiiiiiiiipf..zzz&w
w&.....&iiiiiiiiiii#ip...zgz&w
w&&&&&&&&&&&&&&&&&&&&&&&&&&&&w
wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
"""),
]
for _spec in LEVEL_SPECS:
    level(_spec["name"], _spec["blurb"], _spec["art"], _spec["recipe"], _spec["needs"])

if "--sandbox" in sys.argv:  # engine-vs-simulator test levels (with --nosolve --diff)
    LEVELS.clear()
    level("Sandbox A", "mechanics test", """
wwwwwwwwwwwwwwwwwwwwww
w..b....iiiippp..x...w
w.A..f..iiiipbp..s...w
w...b...iiii.....z...w
w..iiiiiii..ppppp....w
w..iii.iiib.p.f.p..g.w
w..iiiiiii..ppppp....w
w....f.......b.....s.w
w..ppp...s...iiii....w
w..pbp..ff...iiii..x.w
wwwwwwwwwwwwwwwwwwwwww
""")
    level("Sandbox C", "sockets, frost walls", """
wwwwwwwwwwwwwwwwwwwwww
w.....wwwwwwwwwwwwwwww
w.A.f..........x.....w
w.....wwwwwwwwwwwwwwww
w.zzz...f.......wwwwww
w.zgz...............ww
w.zzzwwwwwwwwwwwwwwwww
wwwwwwwwwwwwwwwwwwwwww
""")
    level("Sandbox B", "mechanics test 2", """
wwwwwwwwwwwwwwwwwwwwww
wA.....iiiiiiiiii....w
w..f...i.b..ii..i..x.w
w......iiiiiiiiii....w
w..b.....pppp...ii...w
w.....f..p..p..iii.s.w
w..iii...pppp...ii...w
w..ibi.......b...f...w
w..iii..zzz......x...w
w.......zgz..........w
wwwwwwwwwwwwwwwwwwwwww
""")

# ----------------------------------------------------------------------------------------------
# GDY
# ----------------------------------------------------------------------------------------------
def actions():
    return f'''
Actions:
{C.face_action("avatar")}
  # Player move (absolute). Walking onto ice starts a slide; walking into an ice block or brazier pushes
  # it (it may slide too); the avatar then steps into its old cell and keeps sliding if that cell is ice.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{exec: {{Action: slide}}}}]}}
        Dst: {{Object: ice}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{reward: 5}}]}}
        Dst: {{Object: shrine, Commands: [{{remove: true}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: {BLOCKERS}}}
      - Src:
          Object: avatar
          Commands:
            - set_tile: meta.facing
            - mov: _dest
            - if:
                Conditions: {{eq: [pend, 1]}}
                OnTrue:
                  - exec: {{Action: slide}}
        Dst: {{Object: cube, Commands: [{{set: [src.pend, slip]}}, {{cascade: _dest}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: brazier, Commands: [{{cascade: _dest}}]}}
{thing_moves()}
  # Internal: keep going in the same direction (exec without ActionId reuses the move's vector).
  - Name: slide
    InputMapping:
      Internal: true
    Behaviours:
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{exec: {{Action: slide}}}}]}}
        Dst: {{Object: ice}}
      - Src: {{Object: avatar, Commands: [{{mov: _dest}}, {{reward: 5}}]}}
        Dst: {{Object: shrine, Commands: [{{remove: true}}]}}
{thing_moves()}
  # Fire radiates heat to the 8 surrounding cells at the end of every move.
  - Name: heat
    Trigger:
      Type: RANGE_BOX_BOUNDARY
      Range: 1
    Behaviours:
      - Src: {{Object: [brazier, beacon]}}
        Dst: {{Object: [snowman, frostwall, cube], Commands: [{{remove: true}}]}}
'''


def thing_moves():
    """Behaviours shared by `move` (cascaded push) and `slide` for ice blocks and braziers."""
    return f'''      - Src: {{Object: cube, Commands: [{{mov: _dest}}, {{set: [slip, 0]}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: cube, Commands: [{{mov: _dest}}, {{set: [slip, 1]}}, {{exec: {{Action: slide}}}}]}}
        Dst: {{Object: ice}}
      - Src: {{Object: cube, Commands: [{{remove: true}}, {{reward: 1}}]}}
        Dst: {{Object: pond, Commands: [{{change_to: ice}}]}}
      - Src: {{Object: brazier, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: brazier, Commands: [{{remove: true}}]}}
        Dst: {{Object: ice, Commands: [{{change_to: pond}}]}}
      - Src: {{Object: brazier, Commands: [{{remove: true}}]}}
        Dst: {{Object: pond}}
      - Src: {{Object: brazier, Commands: [{{remove: true}}, {{reward: 3}}]}}
        Dst: {{Object: socket, Commands: [{{change_to: beacon}}]}}'''


def objects():
    avatar = C.facing_sprites("oryx/oryx_fantasy/avatars/man1.png", "frostflame-avatar")
    slip = "    Variables:\n      - Name: slip\n        InitialValue: 0\n"
    av_extra = "    Variables:\n      - Name: pend\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION
    return "\nObjects:\n" + C.terrain_objects(TERRAIN_USED) + "".join([
        C.obj("wall", "w", 1, C.tiled("oryx/oryx_fantasy/wall6"), "[0.55, 0.7, 0.75]", "square", 1.0),
        C.obj("ice", "i", 0, C.img(IMG + "ice.png"), "[0.6, 0.85, 0.95]", "square", 1.0),
        C.obj("pond", "p", 0, C.img(C.TG + "water.png"), "[0.1, 0.3, 0.7]", "square", 1.0),
        C.obj("snowman", "s", 1, C.img("gvgai/newset/snowmanwrong.png"), "[0.95, 0.95, 1.0]", "triangle", 0.8),
        C.obj("frostwall", "z", 1, C.img("gvgai/newset/chamber_wall_blue.png"), "[0.7, 0.9, 1.0]", "square", 1.0),
        C.obj("socket", "x", 1, C.img(IMG + "socket.png"), "[0.4, 0.4, 0.5]", "square", 0.7),
        C.obj("beacon", None, 1, C.img(IMG + "beacon.png"), "[1.0, 0.6, 0.1]", "square", 0.7),
        C.obj("shrine", "g", 1, C.img("gvgai/newset/exit2.png"), "[0.9, 0.2, 0.2]", "square", 0.9),
        C.obj("cube", "b", 1, C.img(IMG + "block.png"), "[0.5, 0.8, 1.0]", "square", 0.7, slip),
        C.obj("brazier", "f", 1, C.img(IMG + "brazier.png"), "[1.0, 0.45, 0.0]", "triangle", 0.7),
        C.obj("avatar", "A", 1, C.imgs(avatar), "[0.9, 0.9, 0.3]", "triangle", 1.0, av_extra, tiles=4),
    ])


def header():
    return C.header("Frost & Flame", """
        An elemental push puzzle on a frozen mountain. Ice is slippery: you and ice blocks slide until
        something stops you. Push an ice block into open water to freeze it into an ice bridge. Push a
        fire brazier onto ice to melt that tile into water (the fire goes out), into a beacon socket to
        light it. Fire melts snowmen, frost walls and ice blocks next to it.
        Win by lighting every beacon and reaching the shrine. Lose when too few braziers remain.
        """.replace("        ", ""), "avatar",
        ["and: [{eq: [socket:count, 0]}, {eq: [shrine:count, 0]}]"],
        ["lt: [brazier:count, socket:count]"], background=IMG + "snow.png")


# ----------------------------------------------------------------------------------------------
# Validation
# ----------------------------------------------------------------------------------------------
NEIGH8 = S.NB8


def validate(spec):
    rows = spec["rows"]
    name = spec["name"]
    C.check_rect(name, rows)
    assert len(C.find(rows, "A")) == 1, f"{name}: need one avatar"
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if y in (0, len(rows) - 1) or x in (0, len(r) - 1):
                assert c in WALL_CHARS, f"{name}: open border at {(x, y)}"
            if c in "f" and "--sandbox" not in sys.argv:  # no fire may start next to something meltable (it would melt on move 1)
                for dx, dy in NEIGH8:
                    assert rows[y + dy][x + dx] not in "sbz", f"{name}: brazier at {(x, y)} melts something at once"
    goals = len(C.find(rows, "x")) + len(C.find(rows, "g"))
    assert goals, f"{name}: no goal"
    assert len(C.find(rows, "f")) >= len(C.find(rows, "x")), f"{name}: not enough braziers"


ABLATION_TEXT = {"noice": "ice can't be entered", "slide": "ice isn't slippery", "freeze": "blocks can't freeze water",
                 "heat": "fire doesn't melt", "push_brazier": "braziers can't be pushed", "push_cube": "blocks can't be pushed"}


def solve_level(spec, max_nodes=3_000_000):
    """BFS (frostflame_sim.Fast) for a shortest-in-events solution, re-checked with the reference
    simulator; then every ablation in spec['needs'] and every single ice block / brazier removal must
    make the level unsolvable (exhaustive search, no node cap hit)."""
    t = time.time()
    rows = spec["rows"]
    res = S.Fast(rows, WALL_CHARS).solve(max_nodes)
    assert res, f"{spec['name']}: NO SOLUTION"
    moves, events, nodes = res
    walls, cells, s0 = S.parse(rows, WALL_CHARS)
    ok, _ = S.replay(S.Rules(walls, cells), s0, moves)
    assert ok, f"{spec['name']}: replay mismatch"
    info = dict(nodes=nodes, ablations=[], objects=0)
    for flag in spec["needs"]:
        f = S.Fast(rows, WALL_CHARS, (flag,))
        assert f.solve(max_nodes) is None and not f.capped, f"{spec['name']}: still solvable without '{flag}'"
        info["ablations"].append(flag)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c in "bf":
                rr = list(rows)
                rr[y] = r[:x] + "." + r[x + 1:]
                f = S.Fast(rr, WALL_CHARS)
                assert f.solve(max_nodes) is None and not f.capped, \
                    f"{spec['name']}: {c} at {(x, y)} is not needed"
                info["objects"] += 1
    return moves, events, info, time.time() - t


# ----------------------------------------------------------------------------------------------
# Engine differential check: random walks in sim vs WASM engine
# ----------------------------------------------------------------------------------------------
def diff_check(gdy_path, sols=(), walks=40, length=150, seed=3):
    rng = random.Random(seed)
    jobs = [dict(level=li, moves=m) for li, m in enumerate(sols) if m]  # the solver's solutions too
    for li, spec in enumerate(LEVELS):
        for _ in range(walks):
            mv, d = [], rng.randint(1, 4)
            for _ in range(length):
                if rng.random() < 0.35:
                    d = rng.randint(1, 4)
                mv.append(d)
            jobs.append(dict(level=li, moves=mv))
    jp = Path("/tmp/frostflame_diff_jobs.json")
    jp.write_text(json.dumps(jobs))
    op = Path("/tmp/frostflame_diff_out.json")
    subprocess.run(["node", str(HERE / "frostflame_dump.mjs"), str(gdy_path), str(jp), str(op)], capture_output=True,
                   check=True)
    eng = json.loads(op.read_text())
    names = dict(avatar=0, cube=1, brazier=2, ice=3, pond=4, snowman=5, frostwall=5, socket=6, beacon=7, shrine=8)
    bad = 0
    for job, states in zip(jobs, eng):
        rows = LEVELS[job["level"]]["rows"]
        walls, cells, s = S.parse(rows, WALL_CHARS)
        rules = S.Rules(walls, cells)
        fast = S.Fast(rows, WALL_CHARS)
        fs = fast.start
        for t, est in enumerate(states):
            if t > 0:
                s = rules.move(s, job["moves"][t - 1])
                fs = fast.move(fs, job["moves"][t - 1])
                if fast.to_slow(fs) != s:
                    bad += 1
                    print(f"  FAST/REFERENCE DIFF L{job['level']} step {t}", file=sys.stderr)
                    break
            got = [set() for _ in range(9)]
            for nm, locs in est["objs"].items():
                for loc in locs:
                    got[names[nm]].add(tuple(loc))
            want = [{s[0]}] + [set(x) for x in s[1:]]
            if got != want:
                bad += 1
                diffs = [(S.FIELDS[i], sorted(want[i] - got[i]), sorted(got[i] - want[i])) for i in range(9)
                         if got[i] != want[i]]
                print(f"  DIFF L{job['level']} step {t} move {job['moves'][t - 1] if t else '-'}: "
                      f"(field, sim-only, engine-only) {diffs}", file=sys.stderr)
                print(S.render(LEVELS[job["level"]]["rows"], s, WALL_CHARS), file=sys.stderr)
                break
            if est["term"]:
                won = rules.done(s)
                lost = rules.lost(s)
                exp = "Win" if won else "Lose" if lost else None
                if est["term"] != exp:
                    bad += 1
                    print(f"  TERM DIFF L{job['level']} step {t}: engine {est['term']} sim {exp}", file=sys.stderr)
                break
            if rules.done(s) or rules.lost(s):
                bad += 1
                print(f"  TERM DIFF L{job['level']} step {t}: engine none, sim done/lost", file=sys.stderr)
                break
    ev = collections.Counter()
    for states in eng:
        for a, b in zip(states, states[1:]):
            for k in ("cube", "brazier", "ice", "pond", "snowman", "frostwall", "socket", "shrine"):
                if sorted(map(tuple, a["objs"].get(k, []))) != sorted(map(tuple, b["objs"].get(k, []))):
                    ev[k] += 1
            pa, pb = a["objs"].get("avatar", [[0, 0]])[0], b["objs"].get("avatar", [[0, 0]])[0]
            if abs(pa[0] - pb[0]) + abs(pa[1] - pb[1]) > 1:
                ev["avatar slide"] += 1
    print(f"diff check: {len(jobs)} runs (solutions + random walks) engine vs simulator vs fast solver, {bad} mismatches; engine events seen: {dict(ev)}",
          file=sys.stderr)
    return bad == 0


# ----------------------------------------------------------------------------------------------
def main():
    make_sprites()
    parts = [header()]
    sols = []
    only = [int(a[2:]) for a in sys.argv if a.startswith("-L")]
    for i, spec in enumerate(LEVELS):
        validate(spec)
        ground = C.decorate(spec["rows"], spec["recipe"], seed=100 + i, floor=".")
        parts.append(C.level_block(spec["name"], spec["blurb"], spec["rows"], ground))
        if only and i not in only:
            sols.append([])
            continue
        if "--nosolve" in sys.argv:
            sols.append([])
            continue
        moves, events, info, dt = solve_level(spec)
        sols.append(moves)
        h, w = len(spec["rows"]), len(spec["rows"][0])
        print(f"{i}. {spec['name']:16s} {w}x{h} BFS solution: {events} events (pushes/slides), {len(moves)} moves, "
              f"{info['nodes']} states; unsolvable if {', '.join(ABLATION_TEXT[a] for a in info['ablations'])}; "
              f"all {info['objects']} blocks/braziers needed ({dt:.1f}s)", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(spec["rows"]), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects()
    if not only and "--nosolve" not in sys.argv:
        P.write_solutions(GID, sols)
    C.emit(GID, gdy, {
        "name": "Frost & Flame",
        "tagline": "Slide on ice, freeze water, melt snow - light every beacon.",
        "description": "An elemental push puzzle. Ice makes everything slide, ice blocks freeze water into "
                       "bridges, and fire braziers thaw ice and melt snowmen around them. Combine them to "
                       "light the mountain's beacons and reach the shrine.",
        "tags": ["puzzle", "original"],
        "difficulty": "hard",
        "original": "",
        "accent": "#38bdf8",
        "icon": "🔥",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Beacons unlit", "kind": "remaining", "object": "socket", "tone": "orange"},
                {"label": "Braziers", "kind": "remaining", "object": "brazier", "tone": "amber"},
                {"label": "Ice blocks", "kind": "remaining", "object": "cube", "tone": "sky"}],
        "win_text": "The mountain is ablaze with light!",
        "lose_text": "Not enough fire left for the beacons. Reset and try again.",
    })
    if "--diff" in sys.argv:
        ok = diff_check(C.GAMES_DIR / f"{GID}.yaml", sols)
        sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
