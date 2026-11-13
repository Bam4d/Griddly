#!/usr/bin/env python3
"""Writes /tmp/memekit_anim_test.yaml: a tiny GDY with memekit-animated objects (logo spin on a Z=1
object; an animated Z=0 floor the avatar can stand on, using over=). Checked by memekit_check.mjs."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402

spin = M.mistral_logo_spin()
fire = M.gpu_fire_frames()
rack = [M.server_rack("idle")] + M.rack_frames()  # tile 0 idle, tiles 1..2 blinking
av = M.bufo("party")
a1, i1 = M.animate("logo", 4, 2)
a2, i2 = M.animate("fire", 3, 1, over=("bufo",))
a3, i3 = M.animate("rack", 2, 3, start_tile=1)
MOVE = C.MOVE_INPUTS.replace("    InputMapping", "  - Name: move\n    InputMapping")
OBJS = (C.obj("bufo", "b", 2, C.imgs(av), tiles=4, extra=C.FACE_INITIAL_ACTION) + C.obj("logo", "L", 1, C.imgs(spin), tiles=4, extra=i1)
        + C.obj("fire", "f", 0, C.imgs(fire), tiles=3, extra=i2) + C.obj("rack", "R", 1, C.imgs(rack), tiles=3, extra=i3)
        + C.obj("wall", "w", 1, C.tiled("memes/dcwall")))
gdy = f'''Version: "0.1"
Environment:
  Name: memekit anim test
  Observers:
    Sprite2D:
      TileSize: 24
      BackgroundTile: {M.datacenter_floor()}
    Block2D:
      TileSize: 24
  Player:
    AvatarObject: bufo
  Termination:
    Win:
      - eq: [_steps, 1000]
  Levels:
    - |
      w w w w w w
      w b f L . w
      w . R . . w
      w w w w w w
Actions:
{MOVE}    Behaviours:
      - Src: {{Object: bufo, Commands: [{{set_tile: meta.facing}}, {{mov: _dest}}]}}
        Dst: {{Object: [_empty, fire]}}
{C.face_action("bufo")}{a1}{a2}{a3}Objects:
{OBJS}'''
Path("/tmp/memekit_anim_test.yaml").write_text(gdy)
print("wrote /tmp/memekit_anim_test.yaml")
