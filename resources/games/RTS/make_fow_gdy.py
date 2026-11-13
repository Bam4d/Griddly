"""Generate GriddlyRTS-FoW.yaml ("Griddly RTS: Frontiers"): the rules plus every map in fow_maps.py.

All game logic lives in the generated GDY; this script only writes it. Run it after changing rules
or maps:

    python make_fow_gdy.py            # writes GriddlyRTS-FoW.yaml next to this file

Mechanics
---------
Economy     workers mine minerals (1 per trip, 2 with Mining research), gems (3 per trip), or chop
            forest (1 per trip, opening new paths). Only one load carried at a time.
Relays      neutral relays on the map: a unit standing next to one captures it (enemy relays too).
            Each owned relay pays its owner 1 resource every 6 ticks.
Buildings   base (trains workers), barracks (trains army), lab (research), tower (auto-fires at
            enemies within 3 cells every 3 ticks), wall (cheap blocker). Workers build them.
Units       worker, scout (fast, cheap, weak), trooper (melee), gunner (shoots 2 cells away, over
            water and walls), medic (heals adjacent friendly units), tank (heavy, slow).
Research    Mining, Weapons (+1 damage), Armour (-1 damage taken), Gunnery (unlocks gunners),
            Vehicles (unlocks tanks), Fortify (towers hit harder and reload faster).
Camera      each player's camera is the (cam_x, cam_y) global pair, moved with move_camera.
Victory     destroy the enemy base.
"""
import os
import sys

HERE = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, HERE)
from fow_maps import build_all, level_text  # noqa: E402

OUT = os.path.join(HERE, "GriddlyRTS-FoW.yaml")
# the Studio demo's playable /rts page bundles its own copy (deploys only upload studio/griddly-demo)
STUDIO_OUT = os.path.join(HERE, "..", "..", "..", "studio", "griddly-demo", "apps", "web", "src", "rts", "GriddlyRTS-FoW.yaml")

MON = "oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_"
WF = "oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/"
TW = "oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_"
ITEMS = "oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_"

UNITS = ["worker", "scout", "combat", "marine", "medic", "tank"]
BUILDINGS = ["base", "barracks", "lab", "tower", "wall", "relay_owned"]
OWNED = UNITS + BUILDINGS
# relays are captured, not destroyed
TARGETS = UNITS + [b for b in BUILDINGS if b != "relay_owned"]
WALKABLE = "[ _empty, bridge ]"


def L(items):
    return "[ " + ", ".join(items) + " ]"


# id, global variable, label, cost, ticks, prerequisite, effect
TECHS = [
    (1, "res_mining", "Mining", 8, 8, None, "workers deposit 2 per mineral trip"),
    (2, "res_weapons", "Weapons", 10, 8, None, "+1 damage for all units"),
    (3, "res_armor", "Armour", 10, 8, None, "-1 damage taken by units and buildings"),
    (4, "res_gunnery", "Gunnery", 12, 10, "res_weapons", "unlocks gunners"),
    (5, "res_vehicles", "Vehicles", 12, 10, "res_armor", "unlocks tanks"),
    (6, "res_fortify", "Fortify", 10, 8, "res_armor", "towers deal +2 damage and reload faster"),
]

# unit, map char, hp, damage, cost, built by, requires, extra variables, sprite, block shape, scale
UNIT_DEFS = {
    "worker": ("H", 6, 1, 5, "base", None, [("carrying", 0)], MON + "jelly_d1.png", "circle", 0.5),
    "scout": ("s", 5, 1, 4, "barracks", None, [], MON + "drone_d1.png", "circle", 0.7),
    "combat": ("c", 12, 3, 5, "barracks", None, [], MON + "beast_d1.png", "pentagon", 0.85),
    "marine": ("g", 8, 2, 6, "barracks", "res_gunnery", [], MON + "marine_m_d1.png", "triangle", 0.8),
    "medic": ("e", 7, 1, 6, "barracks", None, [], MON + "scientist_f_d1.png", "circle", 0.8),
    "tank": ("t", 30, 5, 12, "barracks", "res_vehicles", [("cooldown", 0)], MON + "robot_d1.png", "fill", 0.75),
}
# building, map char, hp, cost, sprite, block shape
BUILDING_DEFS = {
    "base": ("A", 50, None, WF + "img324.png", "hexagon"),
    "barracks": ("B", 25, 15, WF + "img320.png", "square"),
    "lab": ("L", 20, 10, ITEMS + "beaker_green.png", "circle"),
    "tower": ("O", 20, 12, MON + "sentry_d1.png", "triangle"),
    "wall": ("X", 15, 2, TW + "crate.png", "square"),
}
DIRS = {1: ("left", (-1, 0), "a"), 2: ("up", (0, -1), "w"), 3: ("right", (1, 0), "d"), 4: ("down", (0, 1), "s")}


def dir_inputs(desc, key, dist=1):
    out = []
    for i, (name, (dx, dy), k) in DIRS.items():
        kb = f"{key}+{k}" if key else k
        out.append(f'        {i}: {{ Description: "{desc.format(d=name)}", VectorToDest: [ {dx * dist}, {dy * dist} ], KeyBinding: "{kb}" }}')
    return "\n".join(out)


DAMAGE_TAIL = """            - eq:
                Arguments: [ res_armor, 2 ]
                Commands:
                  - add: [ dst.health, 1 ]
            - lte:
                Arguments: [ dst.health, 0 ]
                Commands:
                  - remove: true
"""


def attack_dst(extra_damage=""):
    return f"""          Commands:
            # damage = attacker damage (+1 with the attacker's Weapons) (-1 with the defender's Armour)
            - sub: [ dst.health, src.damage ]
            - eq:
                Arguments: [ src.res_weapons, 2 ]
                Commands:
                  - sub: [ dst.health, 1 ]
{extra_damage}{DAMAGE_TAIL}"""


def build_action(name, desc, key, src, obj, cost, requires=None):
    pre = [f"            - gte: [ player_resources, {cost} ]"]
    if requires:
        pre.append(f"            - eq: [ {requires}, 2 ]")
    return f"""  - Name: {name}
    InputMapping:
      Inputs:
{dir_inputs(desc, key)}
    Behaviours:
      - Src:
          ObjectRules: {src}
          Preconditions:
{chr(10).join(pre)}
          Commands:
            - sub: [ player_resources, {cost} ]
            - spawn: {obj}
        Dst:
          ObjectRules: _empty
"""


def owned_object(name, char, variables, sprite, shape, scale, initial=""):
    vs = "".join(f"      - Name: {k}\n        InitialValue: {v}\n" for k, v in variables)
    var_block = f"    Variables:\n{vs}" if vs else ""
    return f"""  - Name: {name}
    MapCharacter: {char}
    Z: 1
{var_block}{initial}    Observers:
      Sprite2D:
        - Image: {sprite}
      Block2D:
        - Shape: {shape}
          Color: PLAYER
          Scale: {scale}
"""


def scenery(name, char, sprite, color, z=0, shape="square", scale=1.0, variables=(), tiles=None):
    vs = "".join(f"      - Name: {k}\n        InitialValue: {v}\n" for k, v in variables)
    var_block = f"    Variables:\n{vs}" if vs else ""
    sprites = tiles or [sprite]
    sp = "".join(f"        - Image: {s}\n" for s in sprites)
    bl = "".join(f"        - Shape: {shape}\n          Color: {color}\n          Scale: {scale}\n" for _ in sprites)
    return f"""  - Name: {name}
    MapCharacter: "{char}"
    Z: {z}
{var_block}    Observers:
      Sprite2D:
{sp}      Block2D:
{bl}"""


def generate():
    maps = build_all()
    tech_vars = "".join(f"    - Name: {var}\n      InitialValue: 0\n      PerPlayer: true\n" for _, var, *_ in TECHS)
    levels = "\n".join(f"    # {name} | {blurb}\n    - |\n{level_text(grid)}" for name, blurb, grid in maps)

    research_inputs = "\n".join(
        f'        {i}: {{ Description: "Research {label} ({cost}, {ticks} ticks{", needs " + pre[4:].capitalize() if pre else ""}): {eff}", '
        f'VectorToDest: [ 0, 0 ], MetaData: {{ tech: {i} }}, KeyBinding: "r+{i}" }}'
        for i, var, label, cost, ticks, pre, eff in TECHS)
    research_behaviours = ""
    for i, var, label, cost, ticks, pre, eff in TECHS:
        prereq = f"\n            - eq: [ {pre}, 2 ]" if pre else ""
        research_behaviours += f"""      - Src:
          ObjectRules: lab
          Preconditions:
            - eq: [ meta.tech, {i} ]
            - eq: [ src.busy, 0 ]
            - eq: [ {var}, 0 ]
            - gte: [ player_resources, {cost} ]{prereq}
          Commands:
            - sub: [ player_resources, {cost} ]
            - set: [ {var}, 1 ]
            - set: [ src.busy, {i} ]
            - exec:
                Action: research_done
                Delay: {ticks}
                ActionId: 1
        Dst:
          ObjectRules: lab
"""
    done_cmds = "".join(f"""            - eq:
                Arguments: [ src.busy, {i} ]
                Commands:
                  - set: [ {var}, 2 ]
""" for i, var, *_ in TECHS)

    unit_builds = "".join(
        build_action(f"build_{u}" if u != "combat" else "build_combat",
                     f"Train {u if u not in ('combat', 'marine') else {'combat': 'trooper', 'marine': 'gunner'}[u]} ({{d}} of {src})",
                     {"worker": "1", "scout": "z", "combat": "c", "marine": "g", "medic": "m", "tank": "t"}[u],
                     src, u, cost, req) + "\n"
        for u, (char, hp, dmg, cost, src, req, extra, sprite, shape, scale) in UNIT_DEFS.items())
    building_builds = "".join(
        build_action(f"build_{b}", f"Build {b} {{d}} of worker",
                     {"barracks": "b", "lab": "l", "tower": "o", "wall": "x"}[b], "worker", b, cost) + "\n"
        for b, (char, hp, cost, sprite, shape) in BUILDING_DEFS.items() if cost is not None)

    unit_objects = "".join(
        owned_object(u, char, [("health", hp), ("max_health", hp), ("damage", dmg)] + extra, sprite, shape, scale)
        for u, (char, hp, dmg, cost, src, req, extra, sprite, shape, scale) in UNIT_DEFS.items())
    building_objects = ""
    for b, (char, hp, cost, sprite, shape) in BUILDING_DEFS.items():
        variables = [("health", hp), ("max_health", hp)]
        initial = ""
        if b == "base":
            initial = "    InitialActions:\n      - Action: init_camera\n        ActionId: 1\n"
        if b == "lab":
            variables.append(("busy", 0))
        if b == "tower":
            variables.append(("loaded", 1))
        building_objects += owned_object(b, char, variables, sprite, shape, 1.0, initial)

    gdy = f'''Version: "0.1"
Environment:
  Name: GriddlyRTS-FoW
  Description: |
    Griddly RTS: Frontiers. Two players, large maps in many styles, a camera per player and a
    global mini-map. Generated by make_fow_gdy.py (rules) and fow_maps.py (maps): edit those, not
    this file.

    The GDY holds all of the game logic; players (human UI or the scripted AI in fow_policy.py)
    only issue the external actions below, one per unit per tick, and advance time.

    Economy   workers mine minerals (1 per trip, 2 with Mining), gems (3 per trip) or chop forest
              (1 per trip; clears the tile). One load at a time; deposit at your base.
    Relays    stand next to a relay to capture it (enemy relays too); each relay you own pays 1
              resource every 6 ticks.
    Buildings base, barracks, lab (research), tower (auto-fires at enemies within 3 cells), wall.
    Units     worker, scout (fast + cheap), trooper, gunner (shoots 2 cells, over water/walls),
              medic (heals), tank (heavy, moves every other tick).
    Research  Mining, Weapons, Armour, Gunnery (needs Weapons), Vehicles (needs Armour),
              Fortify (needs Armour): stronger towers.
    Victory   destroy the enemy base.
  Observers:
    Block2D:
      TileSize: 6
      BackgroundColor: [ 0.12, 0.13, 0.11 ]
    Sprite2D:
      TileSize: 16
      BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_regolith_a.png
      # a soft player-coloured shadow under every owned object. Colours are egocentric:
      # in a player's own view their objects use Colors[0] (green), the enemy Colors[1].
      HighlightPlayers: true
      HighlightStyle: SHADOW
      HighlightScale: 1.6
    Vector:
      IncludePlayerId: true
      IncludeVariables: true
  Variables:
    - Name: player_resources
      InitialValue: 12
      PerPlayer: true
    # per-player camera centre (moved with move_camera); bounds come from the map_size marker
    - Name: cam_x
      InitialValue: 0
      PerPlayer: true
    - Name: cam_y
      InitialValue: 0
      PerPlayer: true
    - Name: map_w
      InitialValue: 0
    - Name: map_h
      InitialValue: 0
    # research state per player: 0 = not researched, 1 = in progress, 2 = done
{tech_vars}  Player:
    Count: 2
    # egocentric player colours: each player sees itself as the first colour (green) and the
    # enemy as the second. The global mini-map uses player 1 / player 2 directly.
    Colors:
      - [ 0.2, 0.9, 0.3 ]
      - [ 0.95, 0.3, 0.25 ]
  Termination:
    Lose:
      - Conditions:
          - eq: [ base:count, 0 ]
        Reward: -10
        OpposingReward: 10
  Levels:
{levels}

Actions:
  # Camera: pan the player's view. Issued on the player's base; does not move any object.
  - Name: move_camera
    InputMapping:
      Inputs:
        1: {{ Description: Pan camera left, VectorToDest: [ 0, 0 ], MetaData: {{ dx: -2, dy: 0 }}, KeyBinding: ArrowLeft }}
        2: {{ Description: Pan camera up, VectorToDest: [ 0, 0 ], MetaData: {{ dx: 0, dy: -2 }}, KeyBinding: ArrowUp }}
        3: {{ Description: Pan camera right, VectorToDest: [ 0, 0 ], MetaData: {{ dx: 2, dy: 0 }}, KeyBinding: ArrowRight }}
        4: {{ Description: Pan camera down, VectorToDest: [ 0, 0 ], MetaData: {{ dx: 0, dy: 2 }}, KeyBinding: ArrowDown }}
    Behaviours:
      - Src:
          ObjectRules: base
          Commands:
            - add: [ cam_x, meta.dx ]
            - add: [ cam_y, meta.dy ]
            - lt: {{ Arguments: [ cam_x, 0 ], Commands: [ {{ set: [ cam_x, 0 ] }} ] }}
            - lt: {{ Arguments: [ cam_y, 0 ], Commands: [ {{ set: [ cam_y, 0 ] }} ] }}
            - gte: {{ Arguments: [ cam_x, map_w ], Commands: [ {{ set: [ cam_x, map_w ] }}, {{ decr: cam_x }} ] }}
            - gte: {{ Arguments: [ cam_y, map_h ], Commands: [ {{ set: [ cam_y, map_h ] }}, {{ decr: cam_y }} ] }}
        Dst:
          ObjectRules: base

  - Name: move
    InputMapping:
      Inputs:
{dir_inputs("Move {d}", "")}
    Behaviours:
      - Src:
          ObjectRules: [ worker, combat, marine, medic ]
          Commands:
            - mov: _dest
        Dst:
          ObjectRules: {WALKABLE}
      # scouts are fast: each move order takes them two cells when the second cell is free
      - Src:
          ObjectRules: scout
          Commands:
            - mov: _dest
            # no ActionId: the follow-up step reuses this move's direction
            - exec:
                Action: dash
        Dst:
          ObjectRules: {WALKABLE}
      # tanks are slow: after moving they wait for tank_ready before moving again
      - Src:
          ObjectRules: tank
          Preconditions:
            - eq: [ src.cooldown, 0 ]
          Commands:
            - set: [ src.cooldown, 1 ]
            - mov: _dest
            - exec:
                Action: tank_ready
                Delay: 2
                ActionId: 1
        Dst:
          ObjectRules: {WALKABLE}

  # Internal: the scout's second step, in the direction of the move it just made.
  - Name: dash
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ -1, 0 ] }}
        2: {{ VectorToDest: [ 0, -1 ] }}
        3: {{ VectorToDest: [ 1, 0 ] }}
        4: {{ VectorToDest: [ 0, 1 ] }}
    Behaviours:
      - Src:
          ObjectRules: scout
          Commands:
            - mov: _dest
        Dst:
          ObjectRules: {WALKABLE}

  # Gather from an adjacent mineral / gem / forest (one load at a time), or deposit at your base.
  - Name: gather
    InputMapping:
      Inputs:
{dir_inputs("Gather / deposit {d}", "q")}
    Behaviours:
      - Src:
          ObjectRules: worker
          Preconditions:
            - eq: [ src.carrying, 0 ]
          Commands:
            - set: [ src.carrying, 1 ]
        Dst:
          ObjectRules: minerals
          Commands:
            - decr: dst.resources
            - lt: {{ Arguments: [ dst.resources, 10 ], Commands: [ {{ set_tile: 1 }} ] }}
            - lt: {{ Arguments: [ dst.resources, 5 ], Commands: [ {{ set_tile: 2 }} ] }}
            - lte: {{ Arguments: [ dst.resources, 0 ], Commands: [ {{ remove: true }} ] }}
      - Src:
          ObjectRules: worker
          Preconditions:
            - eq: [ src.carrying, 0 ]
          Commands:
            - set: [ src.carrying, 3 ]
        Dst:
          ObjectRules: gems
          Commands:
            - decr: dst.resources
            - lte: {{ Arguments: [ dst.resources, 0 ], Commands: [ {{ remove: true }} ] }}
      - Src:
          ObjectRules: worker
          Preconditions:
            - eq: [ src.carrying, 0 ]
          Commands:
            - set: [ src.carrying, 2 ]
        Dst:
          ObjectRules: forest
          Commands:
            - remove: true
      - Src:
          ObjectRules: worker
          Preconditions:
            - gt: [ src.carrying, 0 ]
            - eq: [ src._playerId, dst._playerId ]
          Commands:
            # carrying: 1 = mineral, 2 = wood, 3 = gem
            - eq: {{ Arguments: [ src.carrying, 1 ], Commands: [ {{ incr: player_resources }} ] }}
            - if:
                Conditions: {{ and: [ {{ eq: [ src.carrying, 1 ] }}, {{ eq: [ res_mining, 2 ] }} ] }}
                OnTrue: [ {{ incr: player_resources }} ]
            - eq: {{ Arguments: [ src.carrying, 2 ], Commands: [ {{ incr: player_resources }} ] }}
            - eq: {{ Arguments: [ src.carrying, 3 ], Commands: [ {{ add: [ player_resources, 3 ] }} ] }}
            - set: [ src.carrying, 0 ]
            - reward: 1
        Dst:
          ObjectRules: base

  # Melee attack on an adjacent enemy unit or building.
  - Name: attack
    InputMapping:
      Inputs:
{dir_inputs("Attack {d}", "f")}
    Behaviours:
      - Src:
          ObjectRules: {L(UNITS)}
          Preconditions:
            - neq: [ src._playerId, dst._playerId ]
          Commands:
            - reward: 1
        Dst:
          ObjectRules: {L(TARGETS)}
{attack_dst()}
  # Gunners shoot an enemy exactly 2 cells away in a straight line (over anything in between).
  - Name: shoot
    InputMapping:
      Inputs:
{dir_inputs("Shoot 2 cells {d}", "x", dist=2)}
    Behaviours:
      - Src:
          ObjectRules: marine
          Preconditions:
            - neq: [ src._playerId, dst._playerId ]
          Commands:
            - reward: 1
        Dst:
          ObjectRules: {L(TARGETS)}
{attack_dst()}
  # Medics heal an adjacent friendly unit by 3 (up to its max health).
  - Name: heal
    InputMapping:
      Inputs:
{dir_inputs("Heal {d}", "h")}
    Behaviours:
      - Src:
          ObjectRules: medic
          Preconditions:
            - eq: [ src._playerId, dst._playerId ]
            - lt: [ dst.health, dst.max_health ]
        Dst:
          ObjectRules: {L(UNITS)}
          Commands:
            - add: [ dst.health, 3 ]
            - gt: {{ Arguments: [ dst.health, dst.max_health ], Commands: [ {{ set: [ dst.health, dst.max_health ] }} ] }}

  # Workers repair an adjacent friendly building by 2 (up to its max health).
  - Name: repair
    InputMapping:
      Inputs:
{dir_inputs("Repair {d}", "e")}
    Behaviours:
      - Src:
          ObjectRules: worker
          Preconditions:
            - eq: [ src._playerId, dst._playerId ]
            - lt: [ dst.health, dst.max_health ]
        Dst:
          ObjectRules: {L(["base", "barracks", "lab", "tower", "wall"])}
          Commands:
            - add: [ dst.health, 2 ]
            - gt: {{ Arguments: [ dst.health, dst.max_health ], Commands: [ {{ set: [ dst.health, dst.max_health ] }} ] }}

  # Builds spawn into the adjacent empty cell in the chosen direction.
{unit_builds}{building_builds}
  # Research is issued on a lab; the tech id is passed as action metadata.
  - Name: research
    InputMapping:
      Inputs:
{research_inputs}
    Behaviours:
{research_behaviours}
  # Internal: a lab finishes its current research project.
  - Name: research_done
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: lab
          Commands:
{done_cmds}            - set: [ src.busy, 0 ]
        Dst:
          ObjectRules: lab

  # Towers fire at an enemy unit within 3 cells whenever loaded (2 damage, 4 with Fortify).
  - Name: tower_fire
    Trigger:
      Type: RANGE_BOX_AREA
      Range: 3
    Behaviours:
      - Src:
          ObjectRules: tower
          Preconditions:
            - eq: [ src.loaded, 1 ]
            - neq: [ src._playerId, dst._playerId ]
          Commands:
            - set: [ src.loaded, 0 ]
            - eq: {{ Arguments: [ res_fortify, 2 ], Commands: [ {{ exec: {{ Action: tower_reload, Delay: 2, ActionId: 1 }} }} ] }}
            - neq: {{ Arguments: [ res_fortify, 2 ], Commands: [ {{ exec: {{ Action: tower_reload, Delay: 3, ActionId: 1 }} }} ] }}
        Dst:
          ObjectRules: {L(UNITS)}
          Commands:
            - sub: [ dst.health, 2 ]
            - eq: {{ Arguments: [ src.res_fortify, 2 ], Commands: [ {{ sub: [ dst.health, 2 ] }} ] }}
{DAMAGE_TAIL}
  - Name: tower_reload
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: tower
          Commands:
            - set: [ src.loaded, 1 ]
        Dst:
          ObjectRules: tower

  # Relays: any unit standing next to a neutral or enemy relay captures it for its player.
  - Name: capture
    Trigger:
      Type: RANGE_BOX_AREA
      Range: 1
    Behaviours:
      - Src:
          ObjectRules: {L(UNITS)}
          Preconditions:
            - neq: [ src._playerId, dst._playerId ]
          Commands:
            - spawn: relay_owned
            - reward: 2
        Dst:
          ObjectRules: [ relay, relay_owned ]
          Commands:
            - remove: true

  # Internal: an owned relay pays its owner every 6 ticks.
  - Name: relay_income
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: relay_owned
          Commands:
            - exec: {{ Action: relay_income, ActionId: 1, Delay: 6 }}
            - incr: player_resources
        Dst:
          ObjectRules: relay_owned

  # Internal: a tank may move again.
  - Name: tank_ready
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: tank
          Commands:
            - set: [ src.cooldown, 0 ]
        Dst:
          ObjectRules: tank

  # Internal: centre each player's camera on their base at the start of the game.
  - Name: init_camera
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: base
          Commands:
            - set: [ cam_x, _x ]
            - set: [ cam_y, _y ]
        Dst:
          ObjectRules: base

  # Internal: the map-size marker (bottom-right corner of every map) records the camera bounds.
  - Name: init_map
    InputMapping:
      Internal: true
      Inputs:
        1: {{ VectorToDest: [ 0, 0 ] }}
    Behaviours:
      - Src:
          ObjectRules: map_size
          Commands:
            - set: [ map_w, _x ]
            - incr: map_w
            - set: [ map_h, _y ]
            - incr: map_h
        Dst:
          ObjectRules: map_size

Objects:
{scenery("minerals", "M", ITEMS + "crystal_green.png", "[ 0.2, 0.9, 0.4 ]", z=1, shape="triangle", variables=[("resources", 15)], tiles=[ITEMS + "crystal_green.png"] * 3)}
{scenery("gems", "G", ITEMS + "crystal_purple.png", "[ 0.8, 0.4, 1.0 ]", z=1, shape="triangle", variables=[("resources", 8)])}
{scenery("forest", "T", "gvgai/oryx/tree2.png", "[ 0.1, 0.35, 0.15 ]", z=1, shape="square", scale=0.9)}
{scenery("rock", "W", "oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_rubble_a.png", "[ 0.45, 0.42, 0.4 ]", z=1)}
{scenery("water", "~", TW + "water.png", "[ 0.15, 0.35, 0.7 ]", z=1)}
{scenery("bridge", "=", "gvgai/newset/logm.png", "[ 0.55, 0.4, 0.25 ]", z=0, scale=0.9)}
{scenery("relay", "R", TW + "generator_off.png", "[ 0.85, 0.85, 0.85 ]", z=1, shape="hexagon", scale=0.8)}
{owned_object("relay_owned", "r", [], TW + "generator_on.png", "hexagon", 0.8, "    InitialActions:" + chr(10) + "      - Action: relay_income" + chr(10) + "        ActionId: 1" + chr(10) + "        Delay: 6" + chr(10))}
  - Name: map_size
    MapCharacter: "?"
    Z: 0
    InitialActions:
      - Action: init_map
        ActionId: 1
    Observers:
      Sprite2D:
        - Image: {TW}floor_regolith_a.png
      Block2D:
        - Shape: square
          Color: [ 0.12, 0.13, 0.11 ]
          Scale: 0.0
{unit_objects}{building_objects}'''
    # minerals use 3 render tiles (full / half / nearly gone)
    gdy = gdy.replace(
        f"        - Image: {ITEMS}crystal_green.png\n        - Image: {ITEMS}crystal_green.png\n        - Image: {ITEMS}crystal_green.png\n",
        f"        - Image: {ITEMS}crystal_green.png\n          Scale: 1.0\n        - Image: {ITEMS}crystal_green.png\n          Scale: 0.75\n        - Image: {ITEMS}crystal_green.png\n          Scale: 0.5\n")
    return gdy, maps


def main():
    gdy, maps = generate()
    for out in (OUT, STUDIO_OUT):
        with open(out, "w") as f:
            f.write(gdy)
        print(f"wrote {out}: {len(maps)} maps")
    for name, _, grid in maps:
        print(f"  {name}: {len(grid[0])}x{len(grid)}")


if __name__ == "__main__":
    main()
