#!/usr/bin/env python3
"""GPU Rich: an isometric datacenter economy game. A new "meme" game (no original).

Bufo collects mayo, pays mayo to install GPUs on server pads, the GPUs mint tokens over time, and bufo
carries the tokens to the Mistral HQ before the runway runs out.

Rules (deterministic for a given seed: no Triggers / Probability; the only randomness is exec
Randomize, which draws from the seeded engine RNG, so recorded runs replay exactly):
  * Mayo: walk into a mayo jar to take it (carry up to 6). Fridges drop a fresh jar next to themselves
    every ~28 ticks.
  * build (directional, B): facing an empty server pad with >= 2 mayo installs a GPU (-2 mayo).
  * Heat, checked per placement (no proximity triggers): the new GPU probes its 4 neighbours (one
    internal `probe` action per direction, explicit ActionIds). Every live GPU neighbour adds 1 heat
    to both. heat 0-1 = cool (render tile 0), 2 = warm (1), 3+ = hot (2). A GPU placed next to 3 or
    more live GPUs overheats at once: it bursts into flames (its neighbours' heat drops back via
    `unprobe`) and after 20 ticks the fire leaves an empty pad (the mayo is lost).
  * GPUs mint a token into a random free neighbouring cell every 14 + 7*heat ticks (hot GPUs are
    slow). Walk into a token to pick it up (carry up to 12).
  * Walk into the Mistral HQ to deliver: the global `tokens` grows by what you carry.
  * Each investor (VC cat) on the level adds 10 tokens to the `target`; each calendar adds 10 runway
    (= 100 ticks). The HQ burns 1 runway every 10 ticks. Win: tokens >= target. Lose: runway hits 0.
  * Le chaton fat (later levels) wanders around the datacenter and eats any token it walks into.

Layout characters: W wall  R server rack  H Mistral HQ  P server pad  F fridge  j mayo jar
  V investor  U calendar  K le chaton fat  A bufo  . floor
Every level is validated (rectangular, one bufo, one HQ, pads/fridges/HQ reachable, target > 0);
bot_gpurich.mjs plays each level in the engine and records the winning runs.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import isokit as I  # noqa: E402
import memekit as M  # noqa: E402

GID = "gpurich"
P = M.PALETTE
WALK = set(".")
ISO = I.ISO
MAYO_CAP, CARRY_CAP, COST = 6, 12, 2


# ----------------------------------------------------------------------------- 2D art (original)
def token2d():
    px = M.blank()
    M.ellipse(px, 12, 12.5, 7.5, 7.5, P["mistral_amber"], P["mistral_yellow"], P["mistral_redorange"])
    M.ellipse(px, 12, 12.5, 5.0, 5.0, P["mistral_orange"], P["mistral_amber"], P["mistral_red"])
    for x, y in [(9, 10), (9, 11), (9, 12), (9, 13), (9, 14), (10, 11), (11, 12), (12, 11), (13, 10), (13, 11),
                 (13, 12), (13, 13), (13, 14), (14, 10), (15, 10), (15, 11), (15, 12), (15, 13), (15, 14)]:
        M.put(px, x - 0, y, P["mistral_yellow"] if x < 14 else P["white"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=21.5, rx=6)
    return M.save("gpurich-token", px)


def fridge2d():
    px = M.blank()
    M.rect(px, 4, 1, 19, 21, (226, 232, 240, 255))
    M.rect(px, 4, 1, 19, 1, P["white"])
    M.rect(px, 18, 2, 19, 21, (170, 180, 196, 255))
    M.rect(px, 4, 8, 19, 8, (140, 150, 168, 255))
    M.rect(px, 6, 3, 6, 6, P["steel_dark"])
    M.rect(px, 6, 10, 6, 15, P["steel_dark"])
    # mayo jar sticker
    M.rect(px, 10, 11, 15, 18, P["mayo"])
    M.rect(px, 10, 10, 15, 11, P["lid"])
    M.rect(px, 10, 14, 15, 14, P["mistral_orange"])
    M.put(px, 14, 3, P["led_blue"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22.6, rx=8)
    return M.save("gpurich-fridge", px)


def pad2d():
    px = M.blank()
    M.rect(px, 2, 4, 21, 20, P["steel_dark"])
    M.rect(px, 3, 5, 20, 19, (52, 56, 70, 255))
    for x in range(4, 20, 3):
        M.rect(px, x, 6, x + 1, 18, (40, 44, 56, 255))
    for x in range(3, 21, 4):
        M.put(px, x, 5, P["mistral_amber"])
        M.put(px, x + 1, 19, P["mistral_amber"])
    M.rect(px, 9, 11, 14, 13, P["led_green"])
    M.outline(px)
    return M.save("gpurich-pad", px)


def calendar2d():
    px = M.blank()
    M.rect(px, 3, 4, 20, 20, P["white"])
    M.rect(px, 3, 4, 20, 8, P["mistral_red"])
    for x in (7, 16):
        M.rect(px, x, 2, x, 5, P["steel_dark"])
    for r in range(3):
        for c in range(4):
            M.rect(px, 5 + c * 4, 10 + r * 3, 6 + c * 4, 11 + r * 3, P["steel"] if (r, c) != (1, 2) else P["mistral_orange"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22.6, rx=8)
    return M.save("gpurich-calendar", px)


def investor2d():
    """A VC cat (pixel cat with a money bag)."""
    w, h, base = C.load_png(C.IMAGES_DIR / M.cat_walk("pixel")[0])
    px = [list(r) for r in base]
    M.ellipse(px, 19, 17, 3.6, 3.8, (120, 160, 70, 255), (160, 200, 100, 255), (80, 110, 50, 255))
    M.rect(px, 18, 12, 20, 13, (80, 110, 50, 255))
    M.put(px, 19, 17, P["mistral_yellow"])
    M.put(px, 19, 16, P["mistral_yellow"])
    M.put(px, 19, 18, P["mistral_yellow"])
    return M.save("gpurich-investor", px)


# ----------------------------------------------------------------------------- iso art
def iso_art():
    floor = I.tint(f"{ISO}/floor-1.png", "remastered/gpurich-iso-floor.png", rgb=(0.85, 0.9, 1.1))
    wall = I.lowered(I.tint(f"{ISO}/wall-grey-1.png", "remastered/gpurich-iso-wall-full.png", rgb=(0.7, 0.78, 1.0)),
                     "remastered/gpurich-iso-wall.png", 8)
    base = I.tint(I.lowered(f"{ISO}/base-1.png", "remastered/gpurich-iso-base-low.png", 6, split=24),
                  "remastered/gpurich-iso-pad.png", rgb=(0.75, 0.8, 1.0))
    bb = lambda src, name, size, foot, dx=0: I.billboard(src, f"remastered/gpurich-bb-{name}.png", size, foot, dx)
    on_pad = lambda src, name, size=20, foot=33: I.composite([(base, 0, 0), (bb(src, name, size, foot), 0, 0)],
                                                            f"remastered/gpurich-iso-{name}.png")
    gpus = [on_pad(M.gpu(s), f"gpu-{s}") for s in ("cool", "warm", "hot")]
    fires = [on_pad(f, f"fire-{i}", 22, 34) for i, f in enumerate(M.gpu_fire_frames())]
    logos = [on_pad(f, f"logo-{i}", 22, 32) for i, f in enumerate(M.mistral_logo_spin())]
    rack = I.composite([(wall, 0, 0), (bb(M.rack_frames()[0], "rack0", 22, 32), 0, 0)], "remastered/gpurich-iso-rack-0.png")
    rack1 = I.composite([(wall, 0, 0), (bb(M.rack_frames()[1], "rack1", 22, 32), 0, 0)], "remastered/gpurich-iso-rack-1.png")
    bufo = [bb(p, f"bufo-{d}", 22, 42) for p, d in zip(M.bufo("plain"), ("up", "right", "down", "left"))]
    return dict(
        floor=floor, wall=wall, pad=base, gpus=gpus, fires=fires, logos=logos, racks=[rack, rack1], bufo=bufo,
        token=bb(token2d(), "token", 14, 40), jar=bb(M.mayo_jar(), "jar", 16, 41), fridge=bb(fridge2d(), "fridge", 26, 42),
        investor=bb(investor2d(), "investor", 22, 42), calendar=bb(calendar2d(), "calendar", 18, 41),
        cats=[bb(p, f"cat-{i}", 24, 43) for i, p in enumerate(M.cat_walk("fat"))],
    )


# ----------------------------------------------------------------------------- levels
def level(rows):
    rows = [r.replace(" ", "") for r in rows]
    return rows


L1 = [  # Seed Round
    "WWWWWWWWWWWWWWWWWWWWWW",
    "W.....U........U.....W",
    "W.F..................W",
    "W.....P...P...P......W",
    "W.j..................W",
    "W.....P...P...P......W",
    "W.j..................W",
    "W.A.......H......V...W",
    "W....................W",
    "W.RR.RR.RR.RR.RR.RR..W",
    "W....................W",
    "WWWWWWWWWWWWWWWWWWWWWW",
]
L2 = [  # Series A - heat
    "WWWWWWWWWWWWWWWWWWWWWWWWWW",
    "W..U.....U.......U.......W",
    "W.F......................W",
    "W.....PPP.......PPP......W",
    "W.j...PPP.......PPP...j..W",
    "W.....PPP.......PPP......W",
    "W........................W",
    "W.A.........H.........V..W",
    "W........................W",
    "W..RRR.RRR.....RRR.RRR.V.W",
    "W.j......................W",
    "W.........F..............W",
    "WWWWWWWWWWWWWWWWWWWWWWWWWW",
]
L3 = [  # Cat in the Datacenter
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    "W..U......U......U......U..W",
    "W.F......................F.W",
    "W....PPPP..........PPPP....W",
    "W....PPPP....K.....PPPP....W",
    "W..........................W",
    "W.RRR.RRR.........RRR.RRR..W",
    "W.j..........H..........j..W",
    "W.RRR.RRR.........RRR.RRR..W",
    "W..........................W",
    "W....PPPP..........PPPP....W",
    "W.A..PPPP..........PPPP..V.W",
    "W.j.....................VV.W",
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWW",
]
L4 = [  # Series C
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    "W.U.....U.....U.....U.....U..W",
    "W.F.........................FW",
    "W...PPPPP...........PPPPP....W",
    "W...PPPPP...RR.RR...PPPPP....W",
    "W...PPPPP...........PPPPP..j.W",
    "W.j..........................W",
    "W.......K.....H.......K......W",
    "W............................W",
    "W.RRRR.RRRR.........RRRR.RRR.W",
    "W............................W",
    "W...PPPPP...........PPPPP....W",
    "W.A.PPPPP.....F.....PPPPP....W",
    "W...PPPPP...........PPPPP.VV.W",
    "W.j......................VV..W",
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
]
L5 = [  # AGI Speedrun
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    "W.U.....U....U....U....U....U..W",
    "W.F...........................FW",
    "W...PPPPPP............PPPPPP...W",
    "W...PPPPPP....RR.RR...PPPPPP...W",
    "W...PPPPPP............PPPPPP.j.W",
    "W.j............................W",
    "W.........K.......K............W",
    "W.RRRR.RRRR.....H.....RRRR.RRR.W",
    "W..............................W",
    "W.......K..............K.......W",
    "W...PPPPPP............PPPPPP...W",
    "W.A.PPPPPP......F.....PPPPPP...W",
    "W...PPPPPP............PPPPPP.V.W",
    "W.j.........................VVVW",
    "W............................VVW",
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
]

LEVELS = [
    ("Seed Round", "Grab mayo, install GPUs on the pads, bring 10 tokens to HQ.", L1),
    ("Series A", "Pads in tight blocks: a GPU next to 3 others overheats. Space them out!", L2),
    ("Cat in the Datacenter", "Le chaton fat eats any token it finds. Collect fast.", L3),
    ("Series C", "Four investors, two cats, big pad farms.", L4),
    ("AGI Speedrun", "Six investors, four cats and a tight runway. Go GPU rich or go home.", L5),
]


def validate(name, rows):
    C.check_rect(name, rows)
    a = C.find(rows, "A")
    assert len(a) == 1, f"{name}: need exactly one bufo"
    assert len(C.find(rows, "H")) == 1, f"{name}: need exactly one HQ"
    seen = C.reach(rows, a[0], lambda c, p: c in WALK or c in "AjK")
    adj = lambda p: [(p[0] + dx, p[1] + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
    for ch in "HFPU":
        for p in C.find(rows, ch):
            if ch == "P":
                # a pad must be buildable: reachable neighbour cell, or a pad neighbour that is (blocks)
                continue
            if ch == "U":
                continue
            assert any(q in seen for q in adj(p)), f"{name}: {ch} at {p} unreachable"
    pads = set(C.find(rows, "P"))
    frontier = {p for p in pads if any(q in seen for q in adj(p))}
    assert frontier, f"{name}: no reachable pad"
    target = 10 * len(C.find(rows, "V"))
    runway = 10 * len(C.find(rows, "U"))
    assert target > 0 and runway > 0, f"{name}: needs investors and calendars"
    return dict(pads=len(pads), fridges=len(C.find(rows, "F")), jars=len(C.find(rows, "j")),
                cats=len(C.find(rows, "K")), target=target, runway_ticks=runway * 10)


# ----------------------------------------------------------------------------- GDY
SOLID = ["wall", "rack", "hq", "fridge", "investor", "calendar", "pad", "gpu", "gpu_fire"]
FLOOR = C.EMPTY_OR_GROUND
ALL = C.GROUND_NAMES + SOLID + ["token", "jar", "cat", "bufo"]
DIRS = [("West", -1, 0), ("North", 0, -1), ("East", 1, 0), ("South", 0, 1)]
DIR_INPUTS = "".join(f"        {i + 1}: {{Description: {d}, VectorToDest: [{x}, {y}]}}\n" for i, (d, x, y) in enumerate(DIRS))


def lst(names):
    return "[" + ", ".join(names) + "]"


HEAT_TILE = ("{if: {Conditions: {lte: [heat, 1]}, OnTrue: [{set_tile: 0}]}}, "
             "{if: {Conditions: {eq: [heat, 2]}, OnTrue: [{set_tile: 1}]}}, "
             "{if: {Conditions: {gte: [heat, 3]}, OnTrue: [{set_tile: 2}]}}")


def actions():
    me = "bufo"
    face = "{set_tile: meta.facing}"
    not_floor = lambda extra=(): lst([o for o in ALL if o not in C.GROUND_NAMES and o not in extra])
    rate = "{set: [rate, 14]}, " + ", ".join(["{add: [rate, heat]}"] * 7)
    return f'''
Actions:
{C.face_action(me)}
  # Investors and calendars set the level's token target and runway at reset.
  - Name: setup
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Setup, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: investor, Commands: [{{add: [target, 10]}}]}}
        Dst: {{Object: [investor, bufo, cat]}}
      - Src: {{Object: calendar, Commands: [{{add: [runway, 10]}}]}}
        Dst: {{Object: [calendar, bufo, cat]}}

  # The HQ burns one unit of runway every 10 ticks.
  - Name: clock
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Tick, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: hq, Commands: [{{decr: runway}}, {{exec: {{Action: clock, ActionId: 1, Delay: 10}}}}]}}
        Dst: {{Object: [hq, bufo, cat]}}

  # A new GPU probes its 4 neighbours once: each live GPU neighbour heats both by 1.
  - Name: probe
    InputMapping:
      Internal: true
      Inputs:
{DIR_INPUTS}    Behaviours:
      - Src: {{Object: gpu, Commands: [{{incr: heat}}]}}
        Dst: {{Object: gpu, Commands: [{{incr: heat}}, {HEAT_TILE}]}}

  # One tick after placement: 3+ live neighbours = meltdown, else show the heat level.
  - Name: settle
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Settle, VectorToDest: [0, 0]}}
    Behaviours:
      - Src:
          Object: gpu
          Commands:
            - if: {{Conditions: {{lte: [heat, 1]}}, OnTrue: [{{set_tile: 0}}]}}
            - if: {{Conditions: {{eq: [heat, 2]}}, OnTrue: [{{set_tile: 1}}]}}
            - if: {{Conditions: {{gte: [heat, 3]}}, OnTrue: [{{decr: gpus}}, {{change_to: gpu_fire}}]}}
        Dst: {{Object: [gpu, bufo, cat]}}

  # A burning GPU cools its neighbours back down, then leaves an empty pad.
  - Name: unprobe
    InputMapping:
      Internal: true
      Inputs:
{DIR_INPUTS}    Behaviours:
      - Src: {{Object: gpu_fire}}
        Dst: {{Object: gpu, Commands: [{{decr: heat}}, {HEAT_TILE}]}}
  - Name: burnout
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: Burn out, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: gpu_fire, Commands: [{{change_to: pad}}]}}
        Dst: {{Object: [gpu_fire, bufo, cat]}}

  # GPUs mint a token into a random free neighbour every 14 + 7*heat ticks.
  - Name: mint
    InputMapping:
      Internal: true
      Inputs:
{DIR_INPUTS}    Behaviours:
      - Src: {{Object: gpu, Commands: [{{spawn: token}}, {rate}, {{exec: {{Action: mint, Delay: rate, Randomize: true}}}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: gpu, Commands: [{{exec: {{Action: mint, Delay: 3, Randomize: true}}}}]}}
        Dst: {{Object: {not_floor()}}}

  # Fridges drop a mayo jar into a random free neighbour every ~28 ticks.
  - Name: chill
    InputMapping:
      Internal: true
      Inputs:
{DIR_INPUTS}    Behaviours:
      - Src: {{Object: fridge, Commands: [{{spawn: jar}}, {{exec: {{Action: chill, Delay: 28, Randomize: true}}}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: fridge, Commands: [{{exec: {{Action: chill, Delay: 5, Randomize: true}}}}]}}
        Dst: {{Object: {not_floor()}}}

  # Le chaton fat wanders and eats any token it walks into.
  - Name: prowl
    InputMapping:
      Internal: true
      Inputs:
{DIR_INPUTS}    Behaviours:
      - Src: {{Object: cat, Commands: [{{mov: _dest}}, {{exec: {{Action: prowl, Delay: 3, Randomize: true}}}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src: {{Object: cat, Commands: [{{mov: _dest}}, {{exec: {{Action: prowl, Delay: 3, Randomize: true}}}}]}}
        Dst: {{Object: token, Commands: [{{remove: true}}]}}
      - Src: {{Object: cat, Commands: [{{exec: {{Action: prowl, Delay: 3, Randomize: true}}}}]}}
        Dst: {{Object: {not_floor(("token",))}}}

  # Bufo walks up / down / left / right. Into a jar: +1 mayo; into a token: +1 carried; into the HQ:
  # deliver every carried token.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: {me}, Commands: [{face}, {{mov: _dest}}]}}
        Dst: {{Object: {FLOOR}}}
      - Src:
          Object: {me}
          Preconditions: [{{lt: [src.mayo, {MAYO_CAP}]}}]
          Commands: [{face}, {{mov: _dest}}, {{incr: mayo}}]
        Dst: {{Object: jar, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Preconditions: [{{lt: [src.carry, {CARRY_CAP}]}}]
          Commands: [{face}, {{mov: _dest}}, {{incr: carry}}, {{reward: 1}}]
        Dst: {{Object: token, Commands: [{{remove: true}}]}}
      - Src:
          Object: {me}
          Commands: [{face}, {{add: [tokens, carry]}}, {{reward: carry}}, {{set: [carry, 0]}}]
        Dst: {{Object: hq}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.mayo, {MAYO_CAP}]}}]
          Commands: [{face}]
        Dst: {{Object: jar}}
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.carry, {CARRY_CAP}]}}]
          Commands: [{face}]
        Dst: {{Object: token}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst([o for o in SOLID if o != "hq"] + ["cat"])}}}

  # Install a GPU on the server pad in front (directional; the UI sends the last move direction).
  - Name: build
{C.MOVE_INPUTS}    Behaviours:
      - Src:
          Object: {me}
          Preconditions: [{{gte: [src.mayo, {COST}]}}]
          Commands: [{face}, {{sub: [mayo, {COST}]}}, {{incr: gpus}}, {{reward: 2}}]
        Dst: {{Object: pad, Commands: [{{change_to: gpu}}]}}
      - Src: {{Object: {me}, Commands: [{face}]}}
        Dst: {{Object: {lst([o for o in ALL if o != me])}}}
'''


def objects():
    art = iso_art()
    v = lambda names: "    Variables:\n" + "".join(f"      - Name: {n}\n        InitialValue: {iv}\n" for n, iv in names)
    bufo_extra = v([("mayo", 0), ("carry", 0)]) + C.FACE_INITIAL_ACTION
    setup = "    InitialActions:\n      - Action: setup\n        ActionId: 1\n"
    probes = "".join(f"      - Action: probe\n        ActionId: {i}\n" for i in range(1, 5))
    gpu_extra = (v([("heat", 0), ("rate", 14)]) + "    InitialActions:\n" + probes +
                 "      - Action: settle\n        ActionId: 1\n        Delay: 1\n"
                 "      - Action: mint\n        Delay: 14\n        Randomize: true\n")
    fire_anim, fire_init = M.animate("gpu_fire", 3, 2)
    unprobes = "".join(f"      - Action: unprobe\n        ActionId: {i}\n" for i in range(1, 5))
    fire_extra = (fire_init + unprobes + "      - Action: burnout\n        ActionId: 1\n        Delay: 20\n")
    hq_anim, hq_init = M.animate("hq", 4, 3)
    hq_extra = hq_init + "      - Action: clock\n        ActionId: 1\n        Delay: 10\n"
    rack_anim, rack_init = M.animate("rack", 2, 4)
    fridge_extra = "    InitialActions:\n      - Action: chill\n        Delay: 10\n        Randomize: true\n"
    cat_anim, cat_init = M.animate("cat", 4, 2)
    cat_extra = cat_init + "      - Action: prowl\n        Delay: 3\n        Randomize: true\n"
    out = [
        I.obj("wall", "W", 1, [C.tiled("memes/dcwall")], [art["wall"]], "[0.3, 0.32, 0.4]", "square", 1.0),
        I.obj("rack", "R", 1, M.rack_frames(), art["racks"], "[0.2, 0.2, 0.25]", "square", 0.95, rack_init),
        I.obj("hq", "H", 1, M.mistral_logo_spin(), art["logos"], "[1.0, 0.55, 0.0]", "square", 1.0, hq_extra),
        I.obj("fridge", "F", 1, [fridge2d()], [art["fridge"]], "[0.9, 0.9, 0.95]", "square", 0.9, fridge_extra),
        I.obj("investor", "V", 1, [investor2d()], [art["investor"]], "[0.3, 0.3, 0.35]", "triangle", 0.8, setup),
        I.obj("calendar", "U", 1, [calendar2d()], [art["calendar"]], "[0.9, 0.2, 0.2]", "square", 0.6, setup),
        I.obj("pad", "P", 1, [pad2d()], [art["pad"]], "[0.25, 0.27, 0.35]", "square", 0.8),
        I.obj("gpu", None, 1, [M.gpu(s) for s in ("cool", "warm", "hot")], art["gpus"], "[0.2, 0.9, 0.3]", "square",
              0.8, gpu_extra, [0.8, 0.8, 0.8]),
        I.obj("gpu_fire", None, 1, M.gpu_fire_frames(), art["fires"], "[1.0, 0.3, 0.0]", "square", 0.8, fire_extra,
              [0.8, 0.8, 0.8]),
        I.obj("token", "t", 1, [token2d()], [art["token"]], "[1.0, 0.7, 0.0]", "circle", 0.4),
        I.obj("jar", "j", 1, [M.mayo_jar()], [art["jar"]], "[1.0, 1.0, 0.85]", "square", 0.5),
        I.obj("cat", "K", 1, M.cat_walk("fat"), art["cats"], "[1.0, 0.55, 0.15]", "triangle", 0.8, cat_extra,
              [0.8] * 4),
        I.obj("bufo", "A", 1, M.bufo("plain"), art["bufo"], "[0.4, 0.7, 0.25]", "triangle", 0.9, bufo_extra,
              [0.9] * 4),
    ]
    anims = fire_anim + hq_anim + rack_anim + cat_anim
    return "\nObjects:\n" + I.terrain(include=set()) + "".join(out), anims, art


def main():
    variables = "".join(f"    - Name: {n}\n      InitialValue: 0\n" for n in ("tokens", "target", "runway", "gpus"))
    objs, anims, art = objects()
    parts = [I.header("GPU Rich", """
        An isometric datacenter economy. Bufo collects mayo, pays 2 mayo to install a GPU on a server pad
        (build), GPUs mint tokens over time, and bufo carries the tokens to the Mistral HQ. A GPU placed
        next to 3 live GPUs overheats and burns; hot GPUs mint slowly. Reach the investors' token target
        before the runway runs out. Le chaton fat eats tokens.
        """.replace("        ", ""), "bufo", ["gte: [tokens, target]"], ["lte: [runway, 0]"],
        background=M.datacenter_floor(), iso_background=art["floor"], variables=variables)]
    for i, (name, blurb, rows) in enumerate(LEVELS):
        info = validate(name, rows)
        parts.append(C.level_block(name, blurb, rows))
        print(f"{i}. {name:22s} {len(rows[0])}x{len(rows)} {info}", file=sys.stderr)
    gdy = "".join(parts) + actions() + anims + objs
    C.emit(GID, gdy, {
        "name": "GPU Rich",
        "tagline": "Turn mayo into GPUs and GPUs into tokens before the runway runs out.",
        "description": "An isometric datacenter tycoon. Bufo trades mayo for GPUs, places them on server pads "
                       "without cooking them (three neighbours = meltdown), hauls the minted tokens to the "
                       "Mistral HQ and keeps the investors happy - while le chaton fat snacks on stray tokens.",
        "tags": ["meme", "economy", "strategy", "isometric"],
        "difficulty": "medium",
        "original": "",
        "accent": "#fb923c",
        "icon": "💰",
        "controls": {"move": "move", "extra": [{"key": "b", "label": "Build GPU", "action": "build",
                                                "directional": True}]},
        "hud": [{"label": "Tokens", "kind": "global", "variable": "tokens", "tone": "amber"},
                {"label": "Target", "kind": "global", "variable": "target", "tone": "orange"},
                {"label": "Runway", "kind": "global", "variable": "runway", "tone": "rose"},
                {"label": "Mayo", "kind": "var", "object": "bufo", "variable": "mayo", "tone": "lime"},
                {"label": "Carrying", "kind": "var", "object": "bufo", "variable": "carry", "tone": "sky"},
                {"label": "GPUs", "kind": "global", "variable": "gpus", "tone": "emerald"}],
        "win_text": "Target smashed. Bufo is officially GPU rich!",
        "lose_text": "The runway ran out. Back to being GPU poor.",
    })


if __name__ == "__main__":
    main()
