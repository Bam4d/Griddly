#!/usr/bin/env python3
"""Timber & Tide: a lumberjack crosses rivers on bridges built from the trees he fells, and turns
wildfire into a tool (new game, not a port).

Rules (the GDY below; lumberjack_sim.py mirrors them exactly for the planner):
  move    walk on ground / bridges / stumps; walking onto a lost HEN rescues it; walking into fire kills.
  chop    (dir, Space) tree -> stump, +1 log (carry at most 3). An OLD OAK takes two chops (the first
          only notches it). Chopping a log pile picks it back up (+1 log); chopping a bush clears it.
  build   (dir, B) with a log: shallow WATER -> bridge (1 log); DEEP water -> bridge (2 logs); on open
          ground it drops a LOG PILE (a solid, flammable block: a fuse to carry fire somewhere).
  bucket  (dir, E) empty bucket + water / deep water / well -> full bucket. Full bucket + fire ->
          doused (a burning bridge is saved and stays a bridge).
  fire    every fire tries to spread to each of its 4 neighbours every 3 ticks: trees, oaks, brambles,
          bushes and log piles catch fire, bridges become burning bridges; anything else (water, rock,
          ground, the lumberjack...) blocks it and it tries again 3 ticks later. A fire burns out 7 ticks
          after it was lit (leaving bare ground; a burning bridge collapses back into water). Campfires
          never burn out. BRAMBLES can't be chopped: only fire clears them.
  goal    rescue every hen. Lose if the lumberjack walks into fire.

Fire is deterministic (exec timers only: no Randomize / Probability / Triggers), so the planner can
simulate it exactly and the recorded solutions replay exactly.

Layout characters (besides the shared terrain chars from common, which include ~ water, & tree,
= bridge, % bush, # rock, $ hedge):
  A lumberjack  h hen  O old oak  D deep water  B bramble  F campfire  f fire  W well  L log pile
Validation per level: rectangular, one avatar; the planner (lumberjack_sim.solve) finds a winning run
which is written to apps/web/src/games/solutions/lumberjack.json; the walk-only check proves no hen is
reachable without chopping/building; ablations re-run the planner without the level's key mechanic
(build / fire-fuse / bucket) and must fail. `--diff` cross-checks the simulator against the engine on
random action sequences (and the recorded solutions).
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
import lumberjack_sim as LS  # noqa: E402

GID = "lumberjack"
HERE = Path(__file__).resolve().parent
SOLUTIONS = C.ROOT / "studio/griddly-demo/apps/web/src/games/solutions" / f"{GID}.json"
IMG = "remastered/lumberjack-"

GROUND = C.GROUND_NAMES  # grass moss dirt sand bones mushroom skull bridge
SCENERY = C.SCENERY_NAMES  # water tree bush rock hedge stone tombstone torch
MINE = ["stump", "oak", "oakcut", "deep", "bramble", "campfire", "fire", "bridgefire", "well", "hen", "logpile"]
ALL = GROUND + SCENERY + MINE + ["avatar"]
FIRES = ["fire", "bridgefire", "campfire"]
FLAMMABLE = ["tree", "oak", "oakcut", "bramble", "logpile", "bush"]
WALK = GROUND + ["stump"]
S, B = LS.S, LS.B


def lst(names):
    return "[" + ", ".join(names) + "]"


# ----------------------------------------------------------------------------------------------
# Sprites
# ----------------------------------------------------------------------------------------------
def _blank():
    return [[(0, 0, 0, 0)] * 24 for _ in range(24)]


def _paste(dst, src, ox=0, oy=0, scale=1.0):
    h, w = len(src), len(src[0])
    for y in range(24):
        for x in range(24):
            sx, sy = int((x - ox) / scale), int((y - oy) / scale)
            if 0 <= sx < w and 0 <= sy < h and src[sy][sx][3] > 0:
                dst[y][x] = src[sy][sx]
    return dst


def make_sprites():
    d = C.IMAGES_DIR
    load = lambda p: C.load_png(d / p)[2]
    fire = load("gvgai/oryx/fire1.png")
    rng = random.Random(3)
    # stump: bark ring with pale rings on top, on transparent ground
    px = _blank()
    for y in range(24):
        for x in range(24):
            dx, dy = (x - 11.5) / 8.0, (y - 13.0) / 5.0
            r = dx * dx + dy * dy
            side = abs(x - 11.5) <= 8 and 13 <= y <= 18
            if r <= 1.0:
                ring = int((r ** 0.5) * 4)
                px[y][x] = [(196, 160, 104, 255), (176, 138, 86, 255), (196, 160, 104, 255), (150, 110, 64, 255),
                            (96, 62, 34, 255)][min(ring, 4)]
            elif side and (abs(x - 11.5) / 8.0) ** 2 + ((y - 18.0) / 5.0) ** 2 <= 1.0 or (side and y < 18):
                px[y][x] = (92, 58, 30, 255) if (x + rng.randint(0, 1)) % 3 else (70, 42, 22, 255)
    C.save_png(d / f"{IMG}stump.png", 24, 24, px)
    # notched oak: tree1 with a pale axe wedge in the trunk
    oak = [list(r) for r in load("gvgai/oryx/tree1.png")]
    for y in range(17, 23):
        for x in range(8, 16):
            if oak[y][x][3] > 0 and abs(x - 12) <= (y - 16) // 2 + 1 and y < 22:
                oak[y][x] = (232, 206, 150, 255)
    C.save_png(d / f"{IMG}oakcut.png", 24, 24, oak)
    # campfire: ring of stones + logs, flame on top
    px = _blank()
    for y in range(24):
        for x in range(24):
            dx, dy = (x - 11.5) / 10.5, (y - 18.0) / 4.5
            r = dx * dx + dy * dy
            if 0.55 <= r <= 1.0:
                px[y][x] = (128, 128, 136, 255) if (x // 3 + y // 2) % 2 else (96, 96, 104, 255)
            elif r < 0.55:
                px[y][x] = (90, 56, 28, 255) if abs(x - y + 6) % 5 > 1 else (60, 36, 18, 255)
    _paste(px, fire, ox=4, oy=-2, scale=0.68)
    C.save_png(d / f"{IMG}campfire.png", 24, 24, px)
    # burning bridge: the log bridge with flames
    px = [list(r) for r in load("gvgai/newset/logm.png")]
    _paste(px, fire, ox=2, oy=-1, scale=0.85)
    C.save_png(d / f"{IMG}bridgefire.png", 24, 24, px)
    # well: stone ring around dark water with a little roof post
    px = _blank()
    for y in range(24):
        for x in range(24):
            dx, dy = (x - 11.5) / 10.5, (y - 14.0) / 8.5
            r = dx * dx + dy * dy
            if r <= 0.5:
                px[y][x] = (30, 70, 140, 255) if (x + y) % 6 else (70, 130, 210, 255)
            elif r <= 1.0:
                px[y][x] = (150, 150, 158, 255) if (x // 3 + y // 3) % 2 else (110, 110, 120, 255)
    for y in range(1, 8):
        for x in (3, 4, 19, 20):
            px[y][x] = (110, 70, 36, 255)
    for x in range(1, 23):
        for y in (0, 1):
            px[y][x] = (150, 60, 40, 255)
    C.save_png(d / f"{IMG}well.png", 24, 24, px)
    # deep water: darker, with white foam streaks (rapids)
    px = []
    rng = random.Random(9)
    for y in range(24):
        row = []
        for x in range(24):
            c = (12 + rng.randint(0, 6), 44 + rng.randint(0, 8), 104 + rng.randint(0, 10), 255)
            if (y * 3 + x // 4) % 9 == 0 and 2 < x < 21:
                c = (190, 220, 245, 255)
            elif (y * 3 + x // 4) % 9 == 1 and 2 < x < 21:
                c = (60, 110, 170, 255)
            row.append(c)
        px.append(row)
    C.save_png(d / f"{IMG}deep.png", 24, 24, px)
    # log pile: three stacked logs (end grain)
    px = _blank()
    for cx, cy in ((7, 17), (16, 17), (11.5, 9.5)):
        for y in range(24):
            for x in range(24):
                r = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
                if r <= 5.2:
                    px[y][x] = (92, 56, 26, 255) if r > 4.2 else ((206, 168, 112, 255) if int(r) % 2 else
                                                                 (180, 140, 88, 255))
    C.save_png(d / f"{IMG}logpile.png", 24, 24, px)
    return {k: f"{IMG}{k}.png" for k in ("stump", "oakcut", "campfire", "bridgefire", "well", "deep", "logpile")}


# ----------------------------------------------------------------------------------------------
# Levels (hand drawn). '.' = plain floor; ground decoration is added by common.decorate.
# ----------------------------------------------------------------------------------------------
LEVELS = [
    ("Riverside", "Chop trees for logs and build a bridge across the river to the hens.", dict(), [
        "$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "$..&&....~~~.....&&......$",
        "$.&&&....~~~.........h...$",
        "$..&......~~.............$",
        "$.........~~~....%%......$",
        "$...A......~~............$",
        "$..&.......~~.....&...#..$",
        "$.&&......~~~........###.$",
        "$.........~~.............$",
        "$..&&.....~~.............$",
        "$.........~~~.....h......$",
        "$..&.......~~............$",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$",
    ]),
    ("Old Oaks & Rapids", "Few trees, two rivers: old oaks take two chops, rapids take two logs.", dict(), [
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "$##.....~~.......D.......h.$",
        "$#......~~.......D.........$",
        "$...&...~~...O...D....##...$",
        "$......~~~.......D.....#...$",
        "$.A.....~~.......DD........$",
        "$.......~~........D....%%..$",
        "$...O...~~...&....D........$",
        "$......~~~........D....h...$",
        "$.......~~........D........$",
        "$..&....~~.......DD........$",
        "$##.....~~.......D......#..$",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
    ]),
    ("Bramble Pen", "Brambles can't be chopped. Lay a fuse of logs from the campfire and let it burn.",
     dict(fuse=[(16, 5), (17, 5), (18, 5)]), [
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "$&&................#######.$",
        "$&.....&...........#.....#.$",
        "$.............@@@..B..h..#.$",
        "$..........,..@....B.....#.$",
        "$....A........@F...B...h.#.$",
        "$..&..........@@@..B.....#.$",
        "$..................B..h..#.$",
        "$..&&..............#.....#.$",
        "$.....&............#######.$",
        "$..&..........~~~..........$",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
    ]),
    ("Bucket Brigade", "A campfire guards the pen: fetch water from the well. Wildfire is eating the wood you need.",
     dict(ablate={"no bucket": ("bucket",), "no build": ("build",)}), [
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "$#####.....&&&&f&&&....~~....$",
        "$#.h.#.....&&&&&&&&....~~..h.$",
        "$#...#.......&&&&&&&...~~....$",
        "$#...#..........&&&&&&.~~....$",
        "$##F##............&&&&&~~....$",
        "$......................~~....$",
        "$......................~~.%%.$",
        "$......................~~....$",
        "$.....W................~~....$",
        "$..A...................~~....$",
        "$....................#.~~....$",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
    ]),
    ("Timber & Tide", "Everything at once: wildfire races for the only bridge over the rapids, an islet, a hut and a bramble pen.",
     dict(fuse=[(32, 15), (32, 14)], stages=[("clear", (20, 12))],
          ablate={"no build": ("build",), "no bucket": ("bucket",)}, ablate_budget=20000, budget=80000), [
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "$&......~~....f&&&&&&&&..DD..#h#.$",
        "$.O.....~~.......&&&&&&..DD..#F#.$",
        "$.......~~..........&&&&.DD.%....$",
        "$.......~~............&&&==......$",
        "$......~~~.............&.DD..##..$",
        "$.A....~~................DD......$",
        "$......~~.....W..........DD..&...$",
        "$..O...~~................DD..%%..$",
        "$.......~~...............DD......$",
        "$.......~~...............DD...O..$",
        "$.......~~........~~~~...DD......$",
        "$.......~~.......~~.h~~..DD#####.$",
        "$..&....~~.......~~~~~...DD#.h.#.$",
        "$.......~~...............DD#...B.$",
        "$.O.....~~...............DD#####.$",
        "$.......~~...............DD.....F$",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
    ]),
]


def level_rows(art):
    rows = [r.replace(" ", "") for r in art]
    C.check_rect("level", rows)
    return rows


# ----------------------------------------------------------------------------------------------
# GDY
# ----------------------------------------------------------------------------------------------
def actions():
    face = "{set_tile: meta.facing}"
    me = "avatar"
    spread_others = [n for n in ALL if n not in FLAMMABLE and n != "bridge"] + ["_empty"]
    chop_dst = ["tree", "oak", "oakcut", "logpile", "bush"]
    build_dst = ["water", "deep"] + WALK
    bucket_dst = ["water", "deep", "well"] + FIRES
    rest = lambda used: lst([n for n in ALL if n not in used])
    return f'''
Actions:
{C.face_action(me)}
  # Fire: every fire tries each neighbour every {S} ticks (one timer per direction).
  - Name: spread
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: West, VectorToDest: [-1, 0]}}
        2: {{Description: North, VectorToDest: [0, -1]}}
        3: {{Description: East, VectorToDest: [1, 0]}}
        4: {{Description: South, VectorToDest: [0, 1]}}
    Behaviours:
      - Src: {{Object: {lst(FIRES)}}}
        Dst: {{Object: {lst(FLAMMABLE)}, Commands: [{{change_to: fire}}]}}
      - Src: {{Object: {lst(FIRES)}}}
        Dst: {{Object: bridge, Commands: [{{change_to: bridgefire}}]}}
      - Src: {{Object: {lst(FIRES)}, Commands: [{{exec: {{Action: spread, Delay: {S}}}}}]}}
        Dst: {{Object: {lst(spread_others)}}}

  # A fire burns out {B} ticks after it was lit; a burning bridge collapses into the river.
  - Name: burnout
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Burn out, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: fire, Commands: [{{remove: true}}]}}
        Dst: {{Object: fire}}
      - Src: {{Object: bridgefire, Commands: [{{change_to: water}}]}}
        Dst: {{Object: bridgefire}}

  # The lumberjack walks up / down / left / right (absolute).
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {lst(["_empty"] + WALK)}}}
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}, {{reward: 5}}]}}
        Dst: {{Object: hen, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{{remove: true}}, {{reward: -5}}]}}
        Dst: {{Object: {lst(FIRES)}}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {rest(WALK + ["hen"] + FIRES)}}}

  # Swing the axe at the cell in the given direction.
  - Name: chop
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{lt: [src.logs, {LS.CAP}]}}]
          Commands: [{face}, {{incr: logs}}, {{reward: 1}}]
        Dst: {{Object: [tree, oakcut], Commands: [{{change_to: stump}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: oak, Commands: [{{change_to: oakcut}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{lt: [src.logs, {LS.CAP}]}}]
          Commands: [{face}, {{incr: logs}}]
        Dst: {{Object: logpile, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: bush, Commands: [{{remove: true}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {rest(chop_dst)}}}

  # Build with logs: a bridge over water (deep water takes 2 logs), or a log pile on open ground.
  - Name: build
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.logs, 1]}}]
          Commands: [{face}, {{decr: logs}}, {{reward: 1}}]
        Dst: {{Object: water, Commands: [{{change_to: bridge}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.logs, 2]}}]
          Commands: [{face}, {{sub: [logs, 2]}}, {{reward: 1}}]
        Dst: {{Object: deep, Commands: [{{change_to: bridge}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.logs, 1]}}]
          Commands: [{face}, {{decr: logs}}, {{spawn: logpile}}]
        Dst: {{Object: {lst(["_empty"] + WALK)}}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {rest(build_dst)}}}

  # Fill the bucket at water or a well; pour it on a fire.
  - Name: bucket
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.bucket, 0]}}]
          Commands: [{face}, {{set: [bucket, 1]}}]
        Dst: {{Object: [water, deep, well]}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.bucket, 1]}}]
          Commands: [{face}, {{set: [bucket, 0]}}, {{reward: 1}}]
        Dst: {{Object: [fire, campfire], Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{eq: [src.bucket, 1]}}]
          Commands: [{face}, {{set: [bucket, 0]}}, {{reward: 1}}]
        Dst: {{Object: bridgefire, Commands: [{{change_to: bridge}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {rest(bucket_dst)}}}
'''


def objects(spr):
    avatar = C.facing_sprites("gvgai/oryx/axeman1.png", "lumberjack-avatar")
    ignite = "".join(f"      - Action: spread\n        ActionId: {i}\n        Delay: {S}\n" for i in (1, 2, 3, 4))
    burn = f"      - Action: burnout\n        ActionId: 1\n        Delay: {B}\n"
    hero = ("    Variables:\n      - Name: logs\n        InitialValue: 0\n      - Name: bucket\n        InitialValue: 0\n"
            + C.FACE_INITIAL_ACTION)
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("stump", None, 0, C.img(spr["stump"]), "[0.5, 0.35, 0.2]", "square", 0.5),
        C.obj("oak", "O", 1, C.img("gvgai/oryx/tree1.png"), "[0.2, 0.5, 0.35]", "triangle", 1.0),
        C.obj("oakcut", None, 1, C.img(spr["oakcut"]), "[0.4, 0.5, 0.35]", "triangle", 0.8),
        C.obj("deep", "D", 1, C.img(spr["deep"]), "[0.05, 0.2, 0.5]", "square", 1.0),
        C.obj("bramble", "B", 1, C.img("gvgai/oryx/bush2.png"), "[0.6, 0.25, 0.1]", "square", 0.9),
        C.obj("campfire", "F", 1, C.img(spr["campfire"]), "[1.0, 0.5, 0.0]", "circle", 0.8,
              "    InitialActions:\n" + ignite),
        C.obj("fire", "f", 1, C.img("gvgai/oryx/fire1.png"), "[1.0, 0.3, 0.0]", "circle", 0.9,
              "    InitialActions:\n" + ignite + burn),
        C.obj("bridgefire", None, 1, C.img(spr["bridgefire"]), "[1.0, 0.4, 0.1]", "circle", 0.9,
              "    InitialActions:\n" + ignite + burn),
        C.obj("well", "W", 1, C.img(spr["well"]), "[0.3, 0.5, 0.9]", "circle", 0.8),
        C.obj("hen", "h", 1, C.img("gvgai/newset/chicken.png"), "[1.0, 1.0, 1.0]", "triangle", 0.6),
        C.obj("logpile", "L", 1, C.img(spr["logpile"]), "[0.7, 0.5, 0.3]", "square", 0.7),
        C.obj("avatar", "A", 2, C.imgs(avatar), "[0.9, 0.2, 0.2]", "triangle", 0.9, hero, tiles=4),
    ])


def header():
    return C.header("Timber & Tide", """
        A lumberjack must rescue the lost hens. Chop trees for logs (carry 3; old oaks take two chops),
        build log bridges over rivers (deep water takes 2 logs) or drop log piles on the ground, and
        carry water in a bucket. Fire spreads to trees, brambles, log piles and bridges every few
        ticks; brambles only burn. Walking into fire is fatal.
        """.replace("        ", ""), "avatar", ["eq: [hen:count, 0]"], ["eq: [avatar:count, 0]"],
        background="gvgai/newset/floor6.png")


# ----------------------------------------------------------------------------------------------
# Validation + solving
# ----------------------------------------------------------------------------------------------
def ablate(s0, hints, banned):
    """Planner with some actions removed: returns True if it still wins."""
    orig = LS.State.step

    def step(self, action, d):
        if action in banned:
            d = 0
        return orig(self, action, d)

    LS.State.step = step
    try:
        plan, _ = LS.solve(s0, hints, max_expand=hints.get("ablate_budget", 4000))
    finally:
        LS.State.step = orig
    return plan is not None


def solve_level(i, name, rows, ground, hints):
    s0 = LS.load(rows, ground)
    hints = dict(hints)
    for k in ("fuse", "douse", "chop"):
        hints[k] = [y * s0.w + x for x, y in hints.get(k, ())]
    stages = []
    for kind, (x, y) in hints.get("stages", ()):
        c = y * s0.w + x
        if kind == "clear":  # the cell (a hen / bramble) is gone
            stages.append(lambda st, c=c: st.mid[c] is None)
    hints["stages"] = stages
    walk = LS.walk_only(s0)
    assert not walk, f"{name}: hens reachable by walking alone {walk}"
    t0 = time.time()
    plan, expanded = LS.solve(s0, hints, max_expand=hints.get("budget", 60000))
    assert plan is not None, f"{name}: planner found no solution ({expanded} expansions)"
    s = s0.copy()
    for a, d in plan:
        s.step(a, d)
    assert s.hens == 0 and s.alive, f"{name}: plan does not win in the simulator"
    abl = {}
    for label, banned in hints.get("ablate", {"no build": ("build",)}).items():
        abl[label] = "WINS?!" if ablate(s0, hints, set(banned)) else "unsolvable"
        assert abl[label] == "unsolvable", f"{name}: still solvable with {label}"
    print(f"  L{i} {name:16s} {len(rows[0])}x{len(rows)} hens={s0.hens} walk-only reachable hens: 0  "
          f"plan {len(plan)} steps ({expanded} expansions, {time.time() - t0:.1f}s)  ablations: {abl}",
          file=sys.stderr)
    return plan


def main():
    spr = make_sprites()
    parts = [header()]
    sols = []
    only = [int(a[2:]) for a in sys.argv if a.startswith("-L")]
    old = json.loads(SOLUTIONS.read_text()) if SOLUTIONS.exists() else None
    for i, (name, blurb, hints, art) in enumerate(LEVELS):
        rows = level_rows(art)
        assert sum(r.count("A") for r in rows) == 1, f"{name}: need one lumberjack"
        ground = C.decorate(rows, hints.get("deco", dict(grass=10, moss=3, dirt=3, mush=3)), seed=4400 + i,
                            stack_under="Ah")
        parts.append(C.level_block(name, blurb, rows, ground))
        if only and i not in only and old and i < len(old["levels"]):
            sols.append(old["levels"][i])
            continue
        plan = solve_level(i, name, rows, ground, hints)
        sols.append({"seed": 1, "actions": [[a, d] for a, d in plan]})
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    gdy = "".join(parts) + actions() + objects(spr)
    SOLUTIONS.parent.mkdir(parents=True, exist_ok=True)
    SOLUTIONS.write_text(json.dumps({"format": 2, "levels": sols}))
    C.emit(GID, gdy, {
        "name": "Timber & Tide",
        "tagline": "Fell trees, bridge rivers, tame wildfire, rescue the hens.",
        "description": "A lumberjack's puzzle-adventure: chop trees for logs, build bridges over rivers (deep "
                       "water costs two logs), lay log fuses to let fire burn through bramble walls and carry "
                       "water to douse the flames before they eat your bridges.",
        "tags": ["puzzle", "crafting", "fire", "original"],
        "difficulty": "medium",
        "original": "new game",
        "accent": "#f59e0b",
        "icon": "🪓",
        "controls": {"move": "move", "extra": [
            {"key": " ", "label": "Chop", "action": "chop", "directional": True},
            {"key": "b", "label": "Build", "action": "build", "directional": True},
            {"key": "e", "label": "Bucket", "action": "bucket", "directional": True}]},
        "hud": [{"label": "Hens", "kind": "count", "object": "hen", "tone": "amber"},
                {"label": "Logs", "kind": "var", "object": "avatar", "variable": "logs", "tone": "orange"},
                {"label": "Bucket", "kind": "var", "object": "avatar", "variable": "bucket", "tone": "sky"}],
        "win_text": "Every hen is safe. Timber!",
        "lose_text": "Caught in the flames.",
    })
    if "--diff" in sys.argv:
        diff(GID)


# ----------------------------------------------------------------------------------------------
# Simulator vs engine
# ----------------------------------------------------------------------------------------------
def snapshot(s: LS.State):
    objs = {}
    for c in range(s.w * s.h):
        for name in (s.mid[c], s.ground[c]):
            if name in ("tree", "oak", "oakcut", "water", "deep", "bramble", "bush", "logpile", "hen", "bridge",
                        "stump") or name in LS.FIRES:
                objs.setdefault(name, []).append([c % s.w, c // s.w])
    if s.alive:
        objs["avatar"] = [[s.pos % s.w, s.pos // s.w]]
    for k in objs:
        objs[k].sort()
    return {"objs": objs, "logs": s.logs if s.alive else None, "bucket": s.bucket if s.alive else None}


def diff(gid, n_random=6, length=260):
    gdy_path = C.GAMES_DIR / f"{gid}.yaml"
    rng = random.Random(5)
    sols = json.loads(SOLUTIONS.read_text())["levels"]
    jobs = []
    for i, (name, blurb, hints, art) in enumerate(LEVELS):
        jobs.append({"level": i, "actions": sols[i]["actions"]})
        for _ in range(n_random):
            acts = []
            for _ in range(length):
                r = rng.random()
                a = "move" if r < 0.55 else "chop" if r < 0.72 else "build" if r < 0.88 else "bucket"
                acts.append([a, rng.randint(0, 4) if a == "move" else rng.randint(1, 4)])
            jobs.append({"level": i, "actions": acts})
    with tempfile.TemporaryDirectory() as td:
        jp, op = Path(td) / "jobs.json", Path(td) / "out.json"
        jp.write_text(json.dumps(jobs))
        subprocess.run(["node", str(HERE / "lumberjack_dump.mjs"), str(gdy_path), str(jp), str(op)], check=True,
                       stdout=subprocess.DEVNULL)
        out = json.loads(op.read_text())
    bad = 0
    for job, states in zip(jobs, out):
        name, blurb, hints, art = LEVELS[job["level"]]
        rows = level_rows(art)
        ground = C.decorate(rows, hints.get("deco", dict(grass=10, moss=3, dirt=3, mush=3)),
                            seed=4400 + job["level"], stack_under="Ah")
        s = LS.load(rows, ground)
        for t, eng in enumerate(states):
            if t > 0:
                s.step(*job["actions"][t - 1])
            mine = snapshot(s)
            if mine != eng:
                bad += 1
                keys = sorted(set(mine["objs"]) | set(eng["objs"]))
                delta = {k: (mine["objs"].get(k), eng["objs"].get(k)) for k in keys
                         if mine["objs"].get(k) != eng["objs"].get(k)}
                print(f"  DIFF level {job['level']} step {t} action {job['actions'][t - 1] if t else None}: "
                      f"logs {mine['logs']}/{eng['logs']} bucket {mine['bucket']}/{eng['bucket']} {delta}",
                      file=sys.stderr)
                break
    total = sum(len(s) for s in out)
    print(f"diff: {len(jobs)} runs, {total} states compared, {bad} mismatching runs", file=sys.stderr)
    if bad:
        sys.exit(1)


if __name__ == "__main__":
    main()
