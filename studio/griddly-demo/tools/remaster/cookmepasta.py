#!/usr/bin/env python3
"""Cook Me Pasta Remastered: push ingredients together through kitchens, restaurants and markets.

Remaster of resources/games/Single-Player/GVGAI/cookmepasta.yaml. Same rules: the chef pushes
ingredients; boiling water + raw pasta -> cooked pasta, tomato + tuna -> pasta sauce, cooked pasta +
sauce -> the complete meal (win). Any other mix of a pasta-side ingredient (water, raw pasta) with a
sauce-side one (tomato, tuna, sauce) ruins the dish (wrong_place: lose). Keys open locks; in the remaster
each key is spent on one lock (the original's `eq: [has_key, 1]` broke with a second key).
Levels are bigger, with counters, tables, barrels, locked pantries and an outdoor market; some have
spare ingredients that must be kept out of the way.

Layout characters (besides the shared terrain chars from common):
  w counter/wall   T table   B barrel   A chef   k key   l lock   . floor
  b boiling water  p raw pasta  o tomato  t tuna
Every level is validated by a best-first search over (chef region, ingredient positions, keys, locks)
that never makes a wrong mix; the move list is printed and written to apps/web/src/games/solutions/
cookmepasta.json so tools/remaster/replay_solutions.mjs can replay it in the engine.

Engine note: the chef is Z=1 like the ingredients. Griddly only blocks `mov` when the destination
holds an object on the same Z layer, so a Z=2 chef would walk on top of an ingredient it failed to push.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import pushlib as P  # noqa: E402

GID = "cookmepasta"
FLOOR = set(".") | set(C.GROUND_CHARS.values())
ITEMS = "bpotCS"  # C cooked pasta, S sauce (solver-internal)
PASTA_SIDE, SAUCE_SIDE = set("bp"), set("otS")
COMBOS = {("b", "p"): "C", ("p", "b"): "C", ("o", "t"): "S", ("t", "o"): "S", ("C", "S"): "M", ("S", "C"): "M"}

# ----------------------------------------------------------------------------- levels
TRATTORIA = """
wwwwwwwwwwwwwwwwwwwwwwwwww
w........................w
w..p........TT.......o...w
w...........TT...........w
w.....wwww........wwww...w
w........................w
w.B.........A..........B.w
w........................w
w.....wwww........wwww...w
w...........TT...........w
w..b........TT.......t...w
w........................w
w.|.........B..........|.w
wwwwwwwwwwwwwwwwwwwwwwwwww
"""

PANTRY = """
wwwwwwwwwwwwwwwwwwwwwwwwwwww
wB......w..................w
w..p....w...T...T...T...T..w
w.......w..................w
w...b...l..................w
w.......w...T...T...T...T..w
wwwwwwwww..................w
w.......w.........A........w
w.......w...T...T...T...T..w
w..o....w..................w
w.......w..................w
w...t...w...T...T...T...k..w
w..........................w
wB......w..................w
w.......w.|.....|.....|....w
wwwwwwwwwwwwwwwwwwwwwwwwwwww
"""

MARKET = """
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&..............................&
&..%%%.....%%%.......%%%..p....&
&..............................&
&....t.........................&
&..%%%.....%%%..~~..%%%........&
&..............~~~~............&
&.......o......~~~~.....b......&
&..%%%..........~~..%%%........&
&..............................&
&.........A....................&
&..%%%.....%%%.......%%%...t...&
&..............................&
&.........................&....&
&..&..................&........&
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
"""

TWO_KITCHENS = """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
w.........w........w...........w
w..b......w...TT...w.......p...w
w.........w...TT...w...........w
w..wwww...l........l...wwww....w
w.........w........w...........w
w.........w...A....w...........w
wwwwwwwwwww........wwwwwwwwwwwww
w.........w...TT...w...........w
w.........w...TT...w.....o.....w
w....t.............w...........w
w.........w....k...w..B.....B..w
w..B...k..w........l...........w
w.|.......w.....k..w.|.......|.w
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
"""

GRAND = """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
w..........w.........w.............w
w...p......w..T...T..w...o.........w
w..........w.........w.......t.....w
w.B.....B..w..T...T..w..wwwww......w
w..........l.........l.............w
wwwwwwwwwwww.........wwwwwwwwwwwwwww
w....................l.............w
w..T...T....T...T....w....t........w
w....................w.............w
w..T...T....A...T....w.B.........B.w
w....................wwwwwwwwlwwwwww
w..T...T....T...T..k...............w
w.............................b....w
w..k...........k...................w
w.|......|.......|.......|.......|.w
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
"""

LEVELS = [
    ("Trattoria", "Boil the pasta, mix the sauce, plate the meal.", TRATTORIA, dict(dirt=2, sand=2)),
    ("The Pantry", "The pasta is locked in the pantry. Find the key in the dining room.", PANTRY, dict(dirt=3)),
    ("Market Day", "Shop the market stalls around the fountain. One tuna too many!", MARKET,
     dict(grass=16, sand=8, dirt=2, mush=2)),
    ("Two Kitchens", "Four locked-off corners: fetch the keys and bring the dish together in the hall.", TWO_KITCHENS,
     dict(dirt=3, sand=2)),
    ("Grand Restaurant", "A dining hall, two kitchens and a cellar of spares. Don't mix them up!", GRAND,
     dict(dirt=4, sand=3)),
]


def parse(text):
    rows = text.strip("\n").split("\n")
    return rows


# ----------------------------------------------------------------------------- solver
class Rules:
    """State: (chef, items: tuple of (pos, kind) sorted, keys: frozenset, locks: frozenset, held)."""

    def __init__(self, rows):
        self.rows = rows
        self.floor = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in FLOOR | set("Abpotkl")}

    def start(self):
        rows = self.rows
        items = tuple(sorted(((x, y), c) for y, r in enumerate(rows) for x, c in enumerate(r) if c in "bpot"))
        keys = frozenset(C.find(rows, "k"))
        locks = frozenset(C.find(rows, "l"))
        return (C.find(rows, "A")[0], items, keys, locks, 0)

    def passable(self, s, cell):
        return cell in self.floor and cell not in s[2] and cell not in s[3] and all(p != cell for p, _ in s[1])

    def step(self, s, cell, m):
        chef, items, keys, locks, held = s
        d = P.DIRS[m]
        n = P.add(cell, d)
        if n in keys:
            return (n, items, keys - {n}, locks, held + 1), False
        if n in locks:
            return ((n, items, keys, locks - {n}, held - 1), False) if held else None
        at = dict(items)
        if n not in at:
            return None
        kind = at[n]
        n2 = P.add(n, d)
        if n2 in at:
            other = at[n2]
            new = COMBOS.get((kind, other))
            if new is None:
                return None  # blocked, or a wrong mix (lose): never do it
            del at[n]
            at[n2] = new
            return (n, tuple(sorted(at.items())), keys, locks, held), new == "M"
        if n2 not in self.floor or n2 in keys or n2 in locks:
            return None
        del at[n]
        at[n2] = kind
        return (n, tuple(sorted(at.items())), keys, locks, held), False

    def done(self, s):
        return False

    def h(self, s):
        by = {}
        for p, k in s[1]:
            by.setdefault(k, []).append(p)
        a, b = self.goal
        if not by.get(a) or not by.get(b):
            return 0
        d = min(abs(x[0] - y[0]) + abs(x[1] - y[1]) for x in by[a] for y in by[b])
        return d + 3 * len(s[2]) + 2 * s[4]  # guidance: also pick up keys / spend them on locks


class Stage(Rules):
    """One sub-goal: combine ingredient kinds `goal` (e.g. ("b", "p")); the stage ends when the product
    appears. The stages chained together are a full solution, since every state is a real game state."""

    def __init__(self, rows, goal):
        super().__init__(rows)
        self.goal = goal
        self.product = COMBOS[goal]

    def step(self, s, cell, m):
        res = Rules.step(self, s, cell, m)
        if res is None:
            return None
        ns, won = res
        before = sum(k == self.product for _, k in s[1])
        after = sum(k == self.product for _, k in ns[1])
        return ns, won or after > before


def staged_solve(rows, max_nodes=20_000, weights=(1.0, 2.0)):
    """Try both stage orders (pasta first / sauce first) and a few heuristic weights."""
    stuck = None
    for order in ((("o", "t"), ("b", "p")), (("b", "p"), ("o", "t"))):
        for w in weights:
            sol, stuck = staged_once(rows, order + (("C", "S"),), max_nodes, w)
            if sol:
                return sol, None
    return None, stuck


def staged_once(rows, goals, max_nodes, weight):
    base = Rules(rows)
    state = base.start()
    moves, events = [], 0
    for goal in goals:
        rules = Stage(rows, goal)
        sol = P.solve(state, rules, max_nodes=max_nodes, weight=weight)
        if not sol:
            return None, goal
        part, ev = sol
        state = run(state, rules, part)
        moves += part
        events += ev
    return (moves, events), None


def run(state, rules, moves):
    for m in moves:
        n = P.add(state[0], P.DIRS[m])
        if rules.passable(state, n):
            state = (n,) + tuple(state[1:])
        else:
            state, _ = rules.step(state, state[0], m)
    return state


def validate(name, rows):
    C.check_rect(name, rows)
    assert len(C.find(rows, "A")) == 1, f"{name}: need exactly one chef"
    for ch in "bpot":
        assert C.find(rows, ch), f"{name}: missing {ch}"
    sol, stuck = staged_solve(rows)
    assert sol, f"{name}: no solution found (stuck at stage {stuck})"
    moves, events = sol
    rules = Rules(rows)
    rules.goal = ("C", "S")
    st = rules.start()
    assert P.replay(st, rules, moves), f"{name}: replay mismatch (full rules)"
    return moves, events


# ----------------------------------------------------------------------------- GDY
INGREDIENTS = ["boiling_water", "raw_pasta", "tomato", "tuna", "cooked_pasta", "pasta_sauce"]
ING = "[" + ", ".join(INGREDIENTS) + "]"


def combo(src, dst, new, reward):
    return (f"      - Src: {{Object: {src}, Commands: [{{remove: true}}, {{reward: {reward}}}]}}\n"
            f"        Dst: {{Object: {dst}, Commands: [{{change_to: {new}}}]}}\n")


ACTIONS = f'''
Actions:
{C.face_action("chef")}
  # The chef walks and pushes ingredients (cascade). Ingredients slide over floor and ground
  # decoration; pushed into each other they combine (or ruin the dish).
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
      - Src: {{Object: chef, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {ING}, Commands: [{{cascade: _dest}}]}}
      - Src: {{Object: chef, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      - Src: {{Object: {ING}, Commands: [{{mov: _dest}}]}}
        Dst: {{Object: {C.EMPTY_OR_GROUND}}}
      # Right combinations
{combo("boiling_water", "raw_pasta", "cooked_pasta", 4)}{combo("raw_pasta", "boiling_water", "cooked_pasta", 4)}\
{combo("tomato", "tuna", "pasta_sauce", 4)}{combo("tuna", "tomato", "pasta_sauce", 4)}\
{combo("cooked_pasta", "pasta_sauce", "complete_meal", 17)}{combo("pasta_sauce", "cooked_pasta", "complete_meal", 17)}\
      # Wrong combinations ruin the dish
{combo("[raw_pasta, boiling_water]", "[tuna, tomato, pasta_sauce]", "wrong_place", -1)}\
{combo("[tuna, tomato, pasta_sauce]", "[boiling_water, raw_pasta]", "wrong_place", -1)}\
      # Keys and locks: a key opens one lock
      - Src: {{Object: chef, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{incr: has_key}}, {{reward: 1}}]}}
        Dst: {{Object: key, Commands: [{{remove: true}}]}}
      - Src:
          Object: chef
          Preconditions: [{{gte: [src.has_key, 1]}}]
          Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}, {{decr: has_key}}]
        Dst: {{Object: lock, Commands: [{{remove: true}}]}}
      - Src: {{Object: chef, Commands: [{{set_tile: meta.facing}}]}}
        Dst: {{Object: [wall, table, barrel, lock, complete_meal, wrong_place, {", ".join(C.SCENERY_NAMES)}]}}
'''


def objects():
    avatar = C.facing_sprites("gvgai/newset/chef.png", "cookmepasta-chef")
    chef_extra = ("    Variables:\n      - Name: has_key\n        InitialValue: 0\n" + C.FACE_INITIAL_ACTION)
    o = lambda name, ch, image, color, shape="triangle", scale=0.8: C.obj(name, ch, 1, C.img(image), color, shape, scale)
    return "\nObjects:\n" + C.terrain_objects() + "".join([
        C.obj("wall", "w", 1, C.WALL_STYLES["brick"], "[0.2, 0.2, 0.6]", "square", 1.0),
        o("table", "T", "gvgai/newset/table1.png", "[0.5, 0.35, 0.2]", "square", 0.8),
        o("barrel", "B", "gvgai/oryx/barrel1.png", "[0.45, 0.3, 0.15]", "circle", 0.8),
        o("key", "k", "gvgai/oryx/key2.png", "[1.0, 0.85, 0.1]", "triangle", 0.5),
        o("lock", "l", "gvgai/newset/lock1.png", "[0.6, 0.6, 0.6]", "square", 0.9),
        o("boiling_water", "b", "gvgai/newset/boilingwater.png", "[0.2, 0.4, 0.9]"),
        o("raw_pasta", "p", "gvgai/newset/pasta.png", "[0.9, 0.85, 0.4]"),
        o("tomato", "o", "gvgai/newset/tomato.png", "[0.9, 0.15, 0.1]"),
        o("tuna", "t", "gvgai/newset/tuna.png", "[0.4, 0.5, 0.7]"),
        o("cooked_pasta", None, "gvgai/newset/pastaplate.png", "[0.95, 0.9, 0.6]"),
        o("pasta_sauce", None, "gvgai/newset/tomatosauce.png", "[0.6, 0.0, 0.2]"),
        o("complete_meal", None, "gvgai/newset/pastasauce.png", "[0.0, 0.9, 0.3]", "square", 0.9),
        o("wrong_place", None, "gvgai/oryx/slash1.png", "[1.0, 0.0, 0.0]", "square", 1.0),
        C.obj("chef", "A", 1, C.imgs(avatar), "[0.3, 0.8, 0.3]", "triangle", 0.9, chef_extra, tiles=4),
    ])


def main():
    parts = [C.header("Cook Me Pasta Remastered", """
        Help the chef cook pasta! Push boiling water into raw pasta to cook it, push tomato into tuna for
        the sauce, then push the cooked pasta and the sauce together to serve the meal. Mixing a pasta
        ingredient with a sauce ingredient ruins the dish. Keys open locked doors (one key per lock).
        """.replace("        ", ""), "chef", ["eq: [complete_meal:count, 1]"], ["eq: [wrong_place:count, 1]"],
        background="gvgai/newset/floor6.png")]
    sols = []
    for i, (name, blurb, text, recipe) in enumerate(LEVELS):
        rows = parse(text)
        moves, events = validate(name, rows)
        sols.append(moves)
        ground = C.decorate(rows, recipe, seed=5100 + i, floor=".", stack_under="Abpotk")
        parts.append(C.level_block(name, blurb, rows, ground))
        print(f"{i}. {name:17s} {len(rows[0])}x{len(rows)} solution: {events} events, {len(moves)} moves",
              file=sys.stderr)
        if "-v" in sys.argv:
            print("   ", "".join(str(m) for m in moves), file=sys.stderr)
    gdy = "".join(parts) + ACTIONS + objects()
    P.write_solutions(GID, sols)  # apps/web/src/games/solutions/<id>.json, read by replay_solutions.mjs
    C.emit(GID, gdy, {
        "name": "Cook Me Pasta Remastered",
        "tagline": "Push the ingredients together — in the right order.",
        "description": "Cook pasta in trattorias, restaurants and a busy market. Boil the pasta, mix tomato "
                       "and tuna into sauce, then bring them together for the perfect meal — but never mix "
                       "pasta with the sauce ingredients. Keys open locked pantries and kitchens.",
        "tags": ["puzzle", "classic-port"],
        "difficulty": "medium",
        "original": "GVGAI/cookmepasta.yaml",
        "accent": "#ef4444",
        "icon": "🍝",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "Keys held", "kind": "var", "object": "chef", "variable": "has_key", "tone": "amber"},
                {"label": "Meals", "kind": "count", "object": "complete_meal", "tone": "emerald"}],
        "win_text": "Buon appetito! The meal is served.",
        "lose_text": "Ruined! Those ingredients don't go together.",
    })


if __name__ == "__main__":
    main()
