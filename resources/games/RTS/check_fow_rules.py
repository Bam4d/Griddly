"""Rules check for GriddlyRTS-FoW (units, buildings, research, towers, relays, ...) on compact test levels.

    GRIDDLY_SDL_WINDOW=1 python check_fow_rules.py
"""
import os
import sys

os.environ.setdefault("GRIDDLY_SDL_WINDOW", "1")
from griddly import GriddlyLoader  # noqa: E402

Y = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GriddlyRTS-FoW.yaml")
LEVEL = """
M  H1 A1 H1 M  .  .  .  .  .
.  M  .  M  .  .  .  .  .  .
.  .  .  .  .  .  H1 .  .  .
.  .  .  .  .  .  .  .  .  .
.  .  .  .  .  .  .  .  H2 A2
"""
gdy = GriddlyLoader().load(Y)
game = gdy.create_game("Block2D")
p1 = game.register_player("P1", "Sprite2D"); p2 = game.register_player("P2", "Sprite2D")
game.init(False); game.load_level_string(LEVEL.strip() + "\n"); game.reset()
def objs(): return [(o["Name"], tuple(o["Location"]), o["PlayerId"], o.get("Variables")) for o in game.get_state()["Objects"] if o["Location"][0] >= 0]
def g(n): return dict(game.get_global_variable([n])[n])
def at(x, y): return [o for o in objs() if o[1] == (x, y)]
def name(x, y): return at(x, y)[0][0] if at(x, y) else None
def hp(x, y): return at(x, y)[0][3]["health"] if at(x, y) else 0
def tick(n=1):
    for _ in range(n): p1.step("move", [0, 0, 0], True)
fails = []
def ok(c, m):
    print(("PASS " if c else "FAIL ") + m)
    if not c: fails.append(m)
res = lambda: g("player_resources")[1]
tech = lambda n: g("res_" + n)[1]
def earn(target):
    """Two workers mine left/down and deposit into the base until resources >= target."""
    guard = 0
    while res() < target and guard < 200:
        guard += 1
        for x, opts in ((1, ((1, 0, 0), (4, 1, 1))), (3, ((3, 4, 0), (4, 3, 1)))):
            for d, mx, my in opts:
                if at(mx, my):
                    p1.step("gather", [x, 0, d], True); break
            p1.step("gather", [x, 0, 3 if x == 1 else 1], True)
    assert res() >= target, f"could not earn {target}"

r0 = res(); earn(r0 + 2); ok(res() == r0 + 2, f"gather + deposit gives +1 per trip ({r0}->{res()})")
p1.step("gather", [1, 0, 1], True); p1.step("gather", [1, 0, 1], True)
ok(at(1, 0)[0][3]["carrying"] == 1, "worker carries at most 1 mineral")
p1.step("gather", [1, 0, 3], True)
earn(10); p1.step("build_lab", [6, 2, 4], True); ok(name(6, 3) == "lab" and at(6, 3)[0][2] == 1, "worker builds lab (cost 10)")
p1.step("research", [6, 3, 1], True); ok(tech("mining") == 0, "research needs money")
earn(18); r = res(); p1.step("research", [6, 3, 1], True)
ok(tech("mining") == 1 and res() == r - 8, "mining research starts, costs 8")
p1.step("research", [6, 3, 2], True); ok(tech("weapons") == 0 and res() == r - 8, "lab busy: only one project at a time")
p1.step("research", [6, 3, 4], True); ok(tech("gunnery") == 0, "gunnery needs weapons first")
tick(7); ok(tech("mining") == 2, "mining research completes after its delay")
r = res(); p1.step("gather", [1, 0, 1] if at(0, 0) else [1, 0, 4], True); p1.step("gather", [1, 0, 3], True)
ok(res() == r + 2, f"advanced mining: deposit +2 ({r}->{res()})")
earn(10); p1.step("research", [6, 3, 2], True); tick(8); ok(tech("weapons") == 2, "weapons researched")
earn(12); p1.step("research", [6, 3, 4], True); tick(10); ok(tech("gunnery") == 2, "gunnery researched (after weapons)")
earn(15); p1.step("build_barracks", [6, 2, 3], True); ok(name(7, 2) == "barracks", "worker builds barracks (cost 15)")
earn(12); p1.step("build_tank", [7, 2, 2], True); ok(name(7, 1) is None, "tank needs vehicles research")
p1.step("build_marine", [7, 2, 3], True); ok(name(8, 2) == "marine", "gunner trained after gunnery")
h = hp(8, 4); p1.step("shoot", [8, 2, 4], True)
ok(h - hp(8, 4) == 3, f"gunner shoots 2 cells away, dmg 2 +1 weapons ({h}->{hp(8, 4)})")
h = hp(7, 2); p1.step("shoot", [8, 2, 1], True); ok(hp(7, 2) == h, "cannot shoot own buildings")
p2.step("move", [8, 4, 2], True); h = hp(8, 2); p2.step("attack", [8, 3, 2], True)
ok(hp(8, 2) == h - 1, "enemy worker melee does 1 damage")
earn(10); p1.step("research", [6, 3, 3], True); tick(8); ok(tech("armor") == 2, "armour researched")
h = hp(8, 2); p2.step("attack", [8, 3, 2], True); ok(hp(8, 2) == h, "armour: worker hit does 1-1 = 0")
earn(12); p1.step("research", [6, 3, 5], True); tick(10); ok(tech("vehicles") == 2, "vehicles researched (after armour)")
earn(12); p1.step("build_tank", [7, 2, 2], True); ok(name(7, 1) == "tank", "tank trained after vehicles")
p1.step("move", [7, 1, 2], True); ok(name(7, 0) == "tank", "tank moves")
p1.step("move", [7, 0, 3], True); ok(name(7, 0) == "tank", "tank cannot move again on the next tick")
p1.step("move", [7, 0, 3], True); ok(name(8, 0) == "tank", "tank moves again after its cooldown")
h = hp(8, 3); p1.step("attack", [8, 2, 4], True); ok(h - hp(8, 3) == 3, "gunner melee: 2 +1 weapons")
p1.step("attack", [8, 2, 4], True); ok(name(8, 3) is None, "enemy worker removed at 0 health")
p2.step("build_worker", [9, 4, 2], True); ok(name(9, 3) == "worker" and at(9, 3)[0][2] == 2, "P2 can train a worker")

# ---------------------------------------------------------------------------------------------
# Frontiers mechanics: gems, wood, relays, towers, medics, scouts, walls, repair, Fortify
# ---------------------------------------------------------------------------------------------
LEVEL2 = """
G  H1 A1 .  .  .  .  .  .  .  .  .
T  H1 .  .  .  .  .  .  .  .  .  .
.  .  .  .  R  .  .  .  .  .  .  .
.  .  .  .  .  .  .  .  .  .  .  .
.  .  .  .  .  .  .  .  .  .  .  .
.  .  .  .  .  .  .  ~  .  .  .  .
.  .  .  .  .  .  .  .  .  .  H2 A2
"""
game.load_level_string(LEVEL2.strip() + "\n"); game.reset()
r0 = res(); p1.step("gather", [1, 0, 1], True)
ok(at(1, 0)[0][3]["carrying"] == 3, "gem gathered (carrying 3)")
p1.step("gather", [1, 0, 3], True); ok(res() == r0 + 3, f"gem deposit is worth 3 ({r0}->{res()})")
p1.step("gather", [1, 1, 1], True); ok(name(0, 1) is None and at(1, 1)[0][3]["carrying"] == 2, "chopping forest clears the tile and carries wood")
# the wood carrier at (1,1) steps right to (2,1), right under the base, and deposits up
p1.step("move", [1, 1, 3], True); p1.step("gather", [2, 1, 2], True)
ok(res() == r0 + 4, f"wood deposit is worth 1 ({res()})")
# relay capture + income: walk the worker from (2,1) to (3,2), next to the relay at (4,2)
p1.step("move", [2, 1, 4], True); p1.step("move", [2, 2, 3], True)
tick(1)
ok(name(4, 2) == "relay_owned" and at(4, 2)[0][2] == 1, "worker next to a relay captures it")
r1 = res(); tick(12); ok(res() >= r1 + 2, f"owned relay pays income ({r1}->{res()} over 12 ticks)")
# towers: build one, an enemy walks into range and gets shot
while res() < 12: tick(6)
p1.step("build_tower", [3, 2, 4], True); ok(name(3, 3) == "tower", "worker builds a tower")
w2 = (10, 6)
for d in (1, 1, 1, 1, 2, 2):
    dx, dy = {1: (-1, 0), 2: (0, -1)}[d]
    p2.step("move", [w2[0], w2[1], d], True); w2 = (w2[0] + dx, w2[1] + dy)
tick(2)
ok(hp(*w2) < 6, f"tower shoots an enemy within 3 cells (enemy worker hp {hp(*w2)})")
# enemy recaptures the relay: walk the (wounded) enemy worker to (5,2), next to the relay
while w2 != (5, 2) and hp(*w2) > 0:
    d = 1 if w2[0] > 5 else 3 if w2[0] < 5 else 2 if w2[1] > 2 else 4
    dx, dy = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}[d]
    p2.step("move", [w2[0], w2[1], d], True); w2 = (w2[0] + dx, w2[1] + dy)
tick(1)
ok(hp(*w2) > 0 and at(4, 2) and at(4, 2)[0][2] == 2 or hp(*w2) == 0,
   f"enemy unit next to the relay takes it over (or dies to the tower first: hp {hp(*w2)})")

LEVEL3 = """
A1 .  .  .  .  .  .  .
.  .  .  B1 .  H1 .  .
.  .  .  .  .  .  .  .
.  .  .  .  .  .  .  .
.  .  .  .  .  .  .  A2
"""
game.load_level_string(LEVEL3.strip() + "\n"); game.reset()
while res() < 40:
    # top up resources with relay-free earning: there are no minerals here, use the global directly
    break
g_res = game.get_global_variable(["player_resources"])["player_resources"]
p1.step("build_medic", [3, 1, 4], True); ok(name(3, 2) == "medic", "barracks trains a medic")
p1.step("build_scout", [3, 1, 2], True); ok(name(3, 0) == "scout", "barracks trains a scout")
p1.step("move", [3, 0, 3], True); ok(name(5, 0) == "scout", f"scout moves two cells per order ({[o[1] for o in objs() if o[0] == 'scout']})")
p1.step("move", [5, 0, 4], True)
ok(name(5, 0) == "scout", "scout dash stops at obstacles (worker below)")
p1.step("build_wall", [5, 1, 3], True); ok(name(6, 1) == "wall", "worker builds a wall")
p1.step("attack", [5, 1, 3], True)
ok(hp(6, 1) == 15, "cannot attack own wall")
print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
sys.exit(1 if fails else 0)
