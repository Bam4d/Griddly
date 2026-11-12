#!/usr/bin/env python3
"""Clusters Remastered: push coloured boxes against the matching static blocks so they cluster.

Remaster of resources/games/Single-Player/GVGAI/clusters.yaml. Same rules: a box pushed against a
static block of its own colour turns into a block (the cluster grows, -1 box_count); a box pushed
onto spikes breaks (lose); walking onto spikes kills the avatar (lose). The global `box_count` is
counted at reset by the internal `box_counter` action every box runs, and the level is won when it
reaches 0. Levels are themed room-and-corridor layouts (pushlib); blocks, boxes and spikes are placed
at random and every candidate is solved forward with a search over (avatar, boxes, grown blocks);
the solvable candidate needing the most pushes is kept. Solutions go to
tools/remaster/solutions/clusters.json for replay_solutions.mjs.

Layout characters (besides the shared terrain chars from common; digits are avoided because the level
parser reads digits as player ids):
  w wall   A avatar   x spikes   r/g/u red/green/blue box   R/G/U red/green/blue block   . floor
"""
from __future__ import annotations

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import pushlib as P  # noqa: E402

GID = "clusters"
COLOURS = {"r": "red", "g": "green", "u": "blue"}
BOX = {c: f"{n}_box" for c, n in COLOURS.items()}
BLOCK = {c: f"{n}_block" for c, n in COLOURS.items()}

LEVELS = [
    dict(name="Toy Room", blurb="Two colours, a few spikes. Push each box against its own colour.", theme="warehouse",
         seed=201, cols=3, rows=2, w=26, h=15, colours="ru", per=2, spikes=4),
    dict(name="Flower Beds", blurb="Three colours of planters in a hedged garden.", theme="garden", seed=212,
         cols=3, rows=2, w=28, h=16, colours="rgu", per=2, spikes=5),
    dict(name="Frozen Pond", blurb="Ice, water and spiky icicles.", theme="frozen", seed=223,
         cols=3, rows=2, w=28, h=16, colours="rgu", per=2, spikes=6),
    dict(name="Bone Vault", blurb="Tombs and spike traps in the crypt.", theme="crypt", seed=234,
         cols=3, rows=2, w=30, h=19, colours="rgu", per=2, spikes=8),
    dict(name="Quarry Works", blurb="Sort the coloured stone in the quarry.", theme="quarry", seed=245,
         cols=4, rows=2, w=32, h=17, colours="rgu", per=2, spikes=8, wide=0.3),
]


class ClusterRules:
    """State = (avatar, frozenset((x, y, colour) boxes), frozenset((x, y, colour) grown blocks))."""

    def __init__(self, floor, blocks, spikes):
        self.floor = set(floor)              # cells free of static stuff (walls, scenery, blocks, spikes)
        self.blocks0 = dict(blocks)          # (x, y) -> colour
        self.spikes = set(spikes)
        self.adj = P.make_adj(self.floor)
        self.dist = {}
        for col in set(blocks.values()):
            goals = [n for n in self.floor for d in P.DIRS.values()
                     if self.blocks0.get(P.add(n, d)) == col and P.add(n, (-d[0], -d[1])) in self.floor]
            self.dist[col] = P.box_distances(self.floor, goals)

    def start(self, player, boxes):
        return (player, frozenset(boxes), frozenset())

    def boxpos(self, s):
        return {(x, y): c for x, y, c in s[1]}

    def blockpos(self, s):
        out = dict(self.blocks0)
        out.update({(x, y): c for x, y, c in s[2]})
        return out

    def passable(self, s, n):
        return n in self.floor and n not in self.blockers(s)

    def blockers(self, s):
        return {(x, y) for x, y, _ in s[1]} | {(x, y) for x, y, _ in s[2]}

    def events(self, s, par):
        return [(P.add((x, y), (-d[0], -d[1])), m) for x, y, _ in s[1] for m, d in P.DIRS.items()
                if P.add((x, y), (-d[0], -d[1])) in par]

    def step(self, s, cell, m):
        d = P.DIRS[m]
        n = P.add(cell, d)
        boxes = self.boxpos(s)
        if n not in boxes:
            return None
        col = boxes[n]
        t = P.add(n, d)
        blocks = self.blockpos(s)
        if blocks.get(t) == col:                           # merge: box becomes a block, avatar stays
            return (cell, s[1] - {(n[0], n[1], col)}, s[2] | {(n[0], n[1], col)}), False
        if t not in self.floor or t in boxes or t in blocks:
            return None                                    # blocked (spikes are not floor: never push there)
        if t not in self.dist[col]:
            return None                                    # can never reach a block of its colour
        nb = s[1] - {(n[0], n[1], col)} | {(t[0], t[1], col)}
        static = lambda p: p not in self.floor or p in blocks
        if (static(P.add(t, (1, 0))) or static(P.add(t, (-1, 0)))) and \
                (static(P.add(t, (0, 1))) or static(P.add(t, (0, -1)))):
            if not any(blocks.get(P.add(t, dd)) == col for dd in P.DIRS.values()):
                return None                                # cornered for good
            # cornered next to its own colour: only ok if it can still be pushed into it
            if not any(blocks.get(P.add(t, dd)) == col and P.add(t, (-dd[0], -dd[1])) in self.floor
                       and P.add(t, (-dd[0], -dd[1])) not in blocks for dd in P.DIRS.values()):
                return None
        return (n, frozenset(nb), s[2]), False

    def done(self, s):
        return not s[1]

    def h(self, s):
        return sum(self.dist[c].get((x, y), 12) + 1 for x, y, c in s[1])


def build(spec):
    m, _, recipe = P.themed_layout(spec["w"], spec["h"], spec["seed"], spec["theme"], spec["cols"], spec["rows"],
                                   rw=(6, 8), rh=spec.get("rh", (4, 6)), wide=spec.get("wide", 0.0), props_scale=0.5)
    fill = P.THEMES[spec["theme"]][0]
    base = ["".join(r) for r in P.crop(m, fill + "w")]
    floor0 = {(x, y) for y, r in enumerate(base) for x, c in enumerate(r) if c == "."}
    best = None
    fails = {"blocks": 0, "boxes": 0, "unsolved": 0}
    for i in range(spec.get("attempts", 30)):
        if sum(fails.values()) - fails["blocks"] - fails["boxes"] >= spec.get("tries", 8):
            break
        rng = P.attempt_rng(spec["seed"], i)
        cells = sorted(floor0)
        rng.shuffle(cells)
        inner = [c for c in cells if sum(P.add(c, d) in floor0 for d in P.DIRS.values()) >= 3]
        blocks = {}
        for col in spec["colours"]:
            p = next(c for c in inner if c not in blocks and sum(P.add(c, d) in floor0 for d in P.DIRS.values()) == 4
                     and all(abs(c[0] - q[0]) + abs(c[1] - q[1]) > 5 for q in blocks))
            blocks[p] = col
        used = set(blocks)
        spikes = []
        floor = set(floor0) - used
        for c in cells:
            if len(spikes) >= spec["spikes"]:
                break
            nb = sum(P.add(c, d) in floor0 for d in P.DIRS.values())
            if c in used or nb != 3 or any(abs(c[0] - q[0]) + abs(c[1] - q[1]) <= 2 for q in used):
                continue  # spikes sit against walls, away from blocks and other spikes
            f2 = floor - {c}
            if len(C.reach(base, next(iter(f2)), lambda ch, p: p in f2)) != len(f2):
                continue
            spikes.append(c)
            used.add(c)
            floor = f2
        if len(C.reach(base, next(iter(floor)), lambda ch, p: p in floor)) != len(floor):
            continue
        boxes = []
        pre = ClusterRules(floor, blocks, spikes)
        if "-d" in sys.argv: print("dist", {k: len(d) for k, d in pre.dist.items()}, len(floor), file=sys.stderr)
        if min(len(d) for d in pre.dist.values()) < len(floor) // 4:
            fails["blocks"] += 1
            continue
        for col in spec["colours"]:
            for _ in range(spec["per"]):
                opts = [c for c in inner if c in floor and c not in used
                        and sum(P.add(c, d) in floor for d in P.DIRS.values()) == 4
                        and all(abs(c[0] - q[0]) + abs(c[1] - q[1]) >= 2 for q in used)
                        and 3 <= pre.dist[col].get(c, 0) <= spec.get("reach", 10)]  # pushable to its colour, not adjacent
                if not opts:
                    break
                c = rng.choice(opts)
                boxes.append((c[0], c[1], col))
                used.add(c)
        if len(boxes) != len(spec["colours"]) * spec["per"]:
            fails["boxes"] += 1
            continue
        player = next(c for c in cells if c in floor and c not in used)
        rules = ClusterRules(floor, blocks, spikes)
        st = rules.start(player, boxes)
        sol = P.solve(st, rules, max_nodes=spec.get("max_nodes", 25_000), weight=spec.get("weight", 2.0))
        if not sol:
            fails["unsolved"] += 1
            continue
        moves, events = sol
        assert P.replay(st, rules, moves), f"{spec['name']}: replay mismatch"
        if best is None or events > best[0]:
            best = (events, moves, blocks, spikes, boxes, player)
    assert best, f"{spec['name']}: no solvable placement {fails}"
    events, moves, blocks, spikes, boxes, player = best
    g = [list(r) for r in base]
    for (x, y), col in blocks.items():
        g[y][x] = col.upper()
    for x, y in spikes:
        g[y][x] = "x"
    for x, y, col in boxes:
        g[y][x] = col
    g[player[1]][player[0]] = "A"
    rows = ["".join(r) for r in g]
    ground = C.decorate(rows, recipe, seed=spec["seed"] + 5000, stack_under="ArguRGUx")
    return rows, ground, moves, events


def validate(name, rows, spec):
    C.check_rect(name, rows)
    assert len(C.find(rows, "A")) == 1, f"{name}: need one avatar"
    for col in spec["colours"]:
        assert C.find(rows, col.upper()) and len(C.find(rows, col)) == spec["per"], f"{name}: colour {col}"


SCENERY = ", ".join(C.SCENERY_NAMES)
BOXES = ", ".join(BOX.values())


def actions():
    merges = "".join(f'''      - Src: {{Object: {BOX[c]}, Commands: [{{change_to: {BLOCK[c]}}}, {{reward: 1}}, {{decr: box_count}}]}}
        Dst: {{Object: {BLOCK[c]}}}
''' for c in COLOURS)
    return f'''
Actions:
{C.face_action("avatar")}
  # Counts the boxes at reset (every box runs it once): the win condition is box_count == 0.
  - Name: box_counter
    InputMapping:
      Internal: true
      Inputs:
        1:
          Description: "The only action here is to increment the box count"
    Behaviours:
      - Src: {{Object: [{BOXES}], Commands: [{{incr: box_count}}]}}
        Dst: {{Object: [{BOXES}]}}

  # Absolute 4-way movement. Walking into a box pushes it. A box pushed against a block of its own
  # colour becomes part of it; a box pushed onto spikes breaks; walking onto spikes is fatal.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: [{BOXES}], Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, {SCENERY}, {", ".join(BLOCK.values())}, broken_box]}}
      - Src: {{Object: avatar, Commands: [{{set_tile: meta.facing}}, {{remove: true}}, {{reward: -1}}]}}
        Dst: {{Object: spike}}
      - Src: {{Object: [{BOXES}], Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
{merges}      - Src: {{Object: [{BOXES}], Commands: [{{change_to: broken_box}}, {{reward: -1}}]}}
        Dst: {{Object: spike}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/oryx/knight1.png", "clusters-avatar")
    count = "    InitialActions:\n      - Action: box_counter\n        ActionId: 1\n"
    out = [
        C.obj("wall", "w", 1, C.tiled("oryx/oryx_fantasy/wall1"), "[0.5, 0.5, 0.5]", "square", 0.9),
        C.obj("spike", "x", 1, C.img("gvgai/oryx/spike2.png"), "[0.9, 0.1, 0.1]", "triangle", 0.5),
    ]
    tint = {"r": ("R", "[0.5, 0.2, 0.2]", "[1.0, 0.0, 0.0]"), "g": ("G", "[0.2, 0.5, 0.2]", "[0.0, 1.0, 0.0]"),
            "u": ("B", "[0.2, 0.2, 0.5]", "[0.0, 0.0, 1.0]")}
    for c in COLOURS:
        k, cb, cl = tint[c]
        out.append(C.obj(BOX[c], c, 2, C.img(f"gvgai/newset/block{k}.png"), cb, "square", 0.5, count))
        out.append(C.obj(BLOCK[c], c.upper(), 2, C.img(f"gvgai/newset/block{k}2.png"), cl, "square", 1.0))
    out.append(C.obj("broken_box", None, 2, C.img("gvgai/newset/block3.png"), "[1.0, 0.0, 1.0]", "triangle", 1.0))
    out.append(C.obj("avatar", "A", 2, C.imgs(avatar), "[0.0, 1.0, 0.0]", "triangle", 0.8, C.FACE_INITIAL_ACTION,
                     tiles=4))
    return "\nObjects:\n" + C.terrain_objects() + "".join(out)


def main():
    parts = [C.header("Clusters Remastered", """
        Cluster the coloured boxes: push each box against the static block of its own colour and it
        becomes part of it. Boxes break on spikes and spikes are deadly to you too. Walls, water,
        hedges, trees, rocks and tombstones block boxes; grass, sand, moss and dirt don't.
        """.replace("        ", ""), "avatar", ["eq: [box_count, 0]"],
        ["gte: [broken_box:count, 1]", "eq: [avatar:count, 0]"], background="oryx/oryx_fantasy/floor1-2.png",
        variables="    - Name: box_count\n      InitialValue: 0\n")]
    sols = []
    for i, spec in enumerate(LEVELS):
        t = time.time()
        rows, ground, moves, events = build(spec)
        validate(spec["name"], rows, spec)
        parts.append(C.level_block(spec["name"], spec["blurb"], rows, ground))
        sols.append(moves)
        print(f"{i}. {spec['name']:13s} {len(rows[0])}x{len(rows)} boxes={len(spec['colours']) * spec['per']} "
              f"solution: {events} pushes, {len(moves)} moves ({time.time() - t:.0f}s)", file=sys.stderr)
        if "-v" in sys.argv:
            print("\n".join(rows), file=sys.stderr)
    P.write_solutions(GID, sols)
    gdy = "".join(parts) + actions() + objects()
    C.emit(GID, gdy, {
        "name": "Clusters Remastered",
        "tagline": "Push every box home to its colour.",
        "description": "Push red, green and blue boxes against the blocks of the same colour to grow the "
                       "clusters. Spikes break boxes and hurt you; scenery is in the way.",
        "tags": ["puzzle", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/clusters.yaml",
        "accent": "#a78bfa",
        "icon": "🧱",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Boxes left", "kind": "global", "variable": "box_count", "tone": "fuchsia"}],
        "win_text": "Every box is clustered!",
        "lose_text": "A box broke (or you stepped on spikes).",
    })


if __name__ == "__main__":
    main()
