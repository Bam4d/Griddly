"""office_kit: small shared helpers for the office-themed meme games (mplace, bufoparty, courier).

* sprite composition on top of memekit art (load a memes/*.png, paste overlays, save under memes/)
* office decor objects: animated server racks, spinning Mistral logos, potted plants
* WALL_16 datacenter wall object, datacenter floor background
* write_solutions2(): format-2 recorded runs ({format: 2, levels: [{seed, actions}]})
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402
import pushlib as P  # noqa: E402

DIRS = {1: (-1, 0), 2: (0, -1), 3: (1, 0), 4: (0, 1)}
FACING = {1: 3, 2: 0, 3: 1, 4: 2}  # move id -> render tile (up, right, down, left order)


def load(rel: str):
    w, h, px = C.load_png(C.IMAGES_DIR / rel)
    return [list(r) for r in px]


def overlay(base_rel: str, name: str, draw) -> str:
    """Load memes sprite `base_rel`, call draw(px) to paint on it, save as memes/<name>.png."""
    px = load(base_rel)
    draw(px)
    return M.save(name, px)


def blend(c, bg, t):
    return tuple(int(c[i] * t + bg[i] * (1 - t)) for i in range(3)) + (255,)


# ------------------------------------------------------------------------------ decor
DECOR_CHARS = {"rack": "Q", "logo": "L", "plant": "P"}


def decor_objects(over_rack=(), over_logo=()) -> tuple[str, str]:
    """(actions_yaml, objects_yaml) for rack (animated LEDs), logo (spinning) and plant decor (solid Z=1)."""
    rack = [M.server_rack("idle")] + M.rack_frames()
    spin = M.mistral_logo_spin()
    a1, i1 = M.animate("rack", 2, 3, start_tile=1)
    a2, i2 = M.animate("logo", 4, 2)
    plant = plant_sprite()
    objs = (C.obj("rack", "Q", 1, C.imgs(rack), "[0.3, 0.3, 0.4]", tiles=3, extra=i1)
            + C.obj("logo", "L", 1, C.imgs(spin), "[1.0, 0.6, 0.0]", tiles=4, extra=i2)
            + C.obj("plant", "P", 1, C.img(plant), "[0.1, 0.6, 0.2]"))
    return a1 + a2, objs


def plant_sprite() -> str:
    px = M.blank()
    M.rect(px, 8, 15, 15, 21, (190, 110, 60, 255))
    M.rect(px, 7, 14, 16, 15, (220, 140, 80, 255))
    for cx, cy, r in ((12, 9, 4.5), (8, 11, 3.2), (16, 11, 3.2), (10, 6, 2.6), (14, 6, 2.6)):
        M.ellipse(px, cx, cy, r, r * 0.9, (70, 170, 80, 255), (130, 210, 120, 255), (40, 110, 50, 255))
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22.5, rx=6)
    return M.save("office-plant", px)


def wall_object(char="w") -> str:
    M.datacenter_wall16()
    return C.obj("wall", char, 1, C.tiled("memes/dcwall"), "[0.3, 0.32, 0.4]", scale=1.0)


def floor_bg() -> str:
    return M.datacenter_floor()


def write_solutions2(game_id: str, runs: list[list[tuple[str, int]]], seed: int = 1):
    P.SOLUTIONS_DIR.mkdir(parents=True, exist_ok=True)
    data = {"format": 2, "levels": [{"seed": seed, "actions": [[a, i] for a, i in run]} for run in runs]}
    (P.SOLUTIONS_DIR / f"{game_id}.json").write_text(json.dumps(data))


def add(p, d):
    return p[0] + d[0], p[1] + d[1]
