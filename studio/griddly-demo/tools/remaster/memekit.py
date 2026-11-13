#!/usr/bin/env python3
"""memekit: shared ORIGINAL pixel art + animation helpers for the "Mistral office meme" games.

Everything here is drawn procedurally by this file (no downloaded / copied images). Every sprite
function writes PNG(s) under resources/images/memes/ (via common.save_png) and returns image path(s)
RELATIVE to resources/images (e.g. "memes/bufo-plain-down.png"), ready for common.img()/imgs().
Sprites are 24x24 RGBA (transparent background unless the tile is a floor/wall/canvas tile).

Sprites
  bufo(variant)            4 facing paths [up, right, down, left] with the yellow facing arrow.
                           variants: plain, gpu (carrying a GPU), mayo (mayo on face), party (party hat)
  bufo_frames(variant)     2 idle frames (bob + blink), no arrow
  cat(variant)             4 facings with arrow. variants: fat (le chaton fat, orange), pixel (tuxedo),
                           sysadmin (grey tabby with headset), guard (brown with a cap)
  cat_walk(variant)        4-frame walk cycle (facing down, no arrow)
  mayo_jar() / mayo_jar_frames() (2, shine) / mayo_splat() (floor decal, mostly transparent)
  gpu(state)               cool | warm | hot | dead ; gpu_fire_frames() 3 flame frames (transparent bg,
                           meant as an overlay object or a "burning gpu" object)
  server_rack(state)       idle | busy | empty ; rack_frames() 2 frames of blinking LEDs
  mistral_logo() / mistral_logo_spin() (4 frames)
  envelope() coolant() cable()
  datacenter_floor()       opaque 24x24 raised-floor tile (use as BackgroundTile or a Z=0 object)
  datacenter_wall16()      16 WALL_16 tiles (index bits: 1 left, 2 right, 4 up, 8 down neighbour)
                           -> use common.tiled("memes/dcwall") in GDY (files memes/dcwall-<i>.png)
  pixel_canvas(color)      opaque canvas tile; color = name in CANVAS_COLORS or "#rrggbb"
  fire_frames()            3 frames of a generic floor fire (same as gpu_fire_frames)

Animation
  animate(obj_name, n_frames, period, start_tile=0, over=())
      -> (actions_yaml, initial_actions_yaml)
      An Internal action `<obj_name>_anim` whose input k (1..n) does `set_tile: start_tile+k-1` and
      re-execs itself with input k%n+1 after `period` ticks (frame/next live in the input MetaData,
      so NO object Variables are needed). Paste actions_yaml under `Actions:` and initial_actions_yaml
      into the object's `extra` (it already starts with `    InitialActions:`; if the object has other
      InitialActions, append only the `      - Action: ...` lines, i.e. initial_actions_yaml minus its
      first line). The object needs `tiles=` >= start_tile+n_frames in common.obj().
      QUIRK: the self-targeted action's destination is the TOP object in the cell. If other objects can
      stand on the animated object (e.g. the avatar on a Z=0 animated floor), list their names in
      `over=` or the animation stops while something stands on it.

Other: PALETTE (named colours), contact_sheet() -> writes memes/_contact_sheet.png (+ .txt index),
validate() round-trips every written PNG. Run this file to (re)generate everything + the sheet.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402

DIR = "memes"
OUT_DIR = C.IMAGES_DIR / DIR
W = H = 24
T = (0, 0, 0, 0)

PALETTE = {
    "outline": (30, 22, 34, 255),
    "shadow": (20, 16, 24, 110),
    "white": (250, 248, 240, 255),
    "black": (24, 22, 28, 255),
    # Mistral-ish stripes (top -> bottom)
    "mistral_yellow": (255, 216, 0, 255),
    "mistral_amber": (255, 175, 0, 255),
    "mistral_orange": (255, 130, 5, 255),
    "mistral_redorange": (250, 80, 15, 255),
    "mistral_red": (225, 5, 0, 255),
    # bufo
    "bufo_hi": (164, 212, 96, 255),
    "bufo": (112, 170, 64, 255),
    "bufo_dark": (66, 118, 50, 255),
    "bufo_belly": (214, 222, 150, 255),
    "bufo_mouth": (70, 34, 40, 255),
    # cats
    "cat_hi": (255, 196, 110, 255),
    "cat": (238, 140, 46, 255),
    "cat_dark": (184, 92, 30, 255),
    "pink": (240, 140, 160, 255),
    # mayo
    "mayo": (250, 240, 200, 255),
    "mayo_hi": (255, 253, 236, 255),
    "mayo_dark": (220, 204, 150, 255),
    "lid": (40, 96, 200, 255),
    "lid_hi": (90, 150, 240, 255),
    # hardware
    "shroud": (44, 46, 56, 255),
    "shroud_hi": (92, 96, 112, 255),
    "shroud_dark": (26, 27, 34, 255),
    "gold": (232, 182, 60, 255),
    "gold_dark": (170, 120, 30, 255),
    "led_green": (80, 240, 110, 255),
    "led_amber": (255, 190, 40, 255),
    "led_red": (255, 60, 50, 255),
    "led_blue": (80, 180, 255, 255),
    "steel": (120, 126, 140, 255),
    "steel_hi": (170, 176, 190, 255),
    "steel_dark": (70, 74, 86, 255),
    "coolant": (60, 200, 230, 255),
    "coolant_hi": (170, 240, 255, 255),
    "floor": (150, 156, 166, 255),
    "floor_hi": (178, 184, 194, 255),
    "floor_dark": (112, 118, 130, 255),
    "wall": (62, 68, 84, 255),
    "wall_hi": (96, 104, 124, 255),
    "wall_dark": (38, 42, 54, 255),
    "flame_y": (255, 236, 120, 255),
    "flame_o": (255, 150, 30, 255),
    "flame_r": (226, 50, 20, 255),
}
P = PALETTE
ARROW_FILL, ARROW_EDGE = (250, 204, 21, 255), (28, 25, 23, 255)
FACINGS = ("up", "right", "down", "left")

CANVAS_COLORS = {
    "white": (238, 236, 228, 255), "black": (40, 38, 46, 255), "red": (225, 40, 40, 255),
    "orange": (255, 130, 5, 255), "yellow": (255, 216, 0, 255), "green": (70, 180, 80, 255),
    "blue": (50, 110, 230, 255), "purple": (140, 70, 200, 255), "pink": (245, 130, 190, 255),
    "brown": (140, 90, 50, 255), "cyan": (60, 200, 220, 255), "grey": (140, 140, 150, 255),
}

_written: dict[str, list] = {}  # rel path -> pixels (this process)
_sheet_order: list[str] = []


# ----------------------------------------------------------------------------------------------
# tiny drawing library
# ----------------------------------------------------------------------------------------------
def blank(fill=T):
    return [[fill] * W for _ in range(H)]


def put(px, x, y, c):
    x, y = int(x), int(y)
    if 0 <= x < W and 0 <= y < H and c is not None:
        px[y][x] = c


def rect(px, x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(px, x, y, c)


def ellipse(px, cx, cy, rx, ry, c, hi=None, dark=None, light=(-0.7, -0.7)):
    """Filled ellipse (pixel centres), shaded: hi on the light side, dark on the far side."""
    for y in range(H):
        for x in range(W):
            dx, dy = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
            if dx * dx + dy * dy <= 1.0:
                col = c
                s = dx * light[0] + dy * light[1]
                if hi and s > 0.45:
                    col = hi
                elif dark and s < -0.55:
                    col = dark
                put(px, x, y, col)


def line(px, x0, y0, x1, y1, c):
    n = max(abs(x1 - x0), abs(y1 - y0), 1)
    for i in range(n + 1):
        put(px, round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n), c)


def tri(px, a, b, c, col):
    for y in range(H):
        for x in range(W):
            if C._in_tri(x + 0.5, y + 0.5, a, b, c):
                put(px, x, y, col)


def outline(px, c=None):
    """1px dark outline around the opaque silhouette (4-neighbourhood)."""
    c = c or P["outline"]
    solid = [[p[3] > 0 for p in row] for row in px]
    for y in range(H):
        for x in range(W):
            if solid[y][x]:
                continue
            if any(0 <= x + dx < W and 0 <= y + dy < H and solid[y + dy][x + dx]
                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[y][x] = c
    return px


def ground_shadow(px, cx=12, cy=21.5, rx=8, ry=1.6):
    for y in range(H):
        for x in range(W):
            dx, dy = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
            if dx * dx + dy * dy <= 1.0 and px[y][x][3] == 0:
                px[y][x] = P["shadow"]


def copy(px):
    return [list(r) for r in px]


def shift(px, dx, dy):
    out = blank()
    for y in range(H):
        for x in range(W):
            if 0 <= x - dx < W and 0 <= y - dy < H:
                out[y][x] = px[y - dy][x - dx]
    return out


def mirror(px):
    return [list(reversed(r)) for r in px]


def paste(dst, src, ox=0, oy=0):
    for y in range(len(src)):
        for x in range(len(src[0])):
            if src[y][x][3] > 0:
                put(dst, x + ox, y + oy, src[y][x])
    return dst


def add_arrow(px, d):
    """The yellow facing arrow at the tile edge, same geometry as common.facing_sprites."""
    out = copy(px)
    for y in range(H):
        for x in range(W):
            cx, cy = x + 0.5, y + 0.5
            u, v = {"up": (cx, cy), "right": (cy, W - cx), "down": (W - cx, H - cy), "left": (H - cy, cx)}[d]
            mid = W / 2.0
            outer = ((mid, 0.0), (mid - 5.0, 6.5), (mid + 5.0, 6.5))
            inner = ((mid, 1.2), (mid - 3.4, 5.6), (mid + 3.4, 5.6))
            if C._in_tri(u, v, *inner):
                out[y][x] = ARROW_FILL
            elif C._in_tri(u, v, *outer):
                out[y][x] = ARROW_EDGE
    return out


def save(name: str, px) -> str:
    rel = f"{DIR}/{name}.png"
    if rel not in _written:
        _sheet_order.append(rel)
    _written[rel] = px
    C.save_png(C.IMAGES_DIR / rel, len(px[0]), len(px), px)
    return rel


def _rgba(c):
    if isinstance(c, str):
        if c in CANVAS_COLORS:
            return CANVAS_COLORS[c]
        if c in P:
            return P[c]
        h = c.lstrip("#")
        return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), 255)
    return tuple(c) + ((255,) if len(c) == 3 else ())


def _mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3)) + (255,)


# ----------------------------------------------------------------------------------------------
# bufo
# ----------------------------------------------------------------------------------------------
_GPU_MINI = None


def _gpu_mini(px, x0, y0, w=10, h=5):
    """A tiny GPU card (held items, hats...)."""
    rect(px, x0, y0, x0 + w - 1, y0 + h - 1, P["shroud"])
    rect(px, x0, y0, x0 + w - 1, y0, P["shroud_hi"])
    for fx in (x0 + 2, x0 + w - 3):
        put(px, fx, y0 + 2, P["steel"])
        put(px, fx - 1, y0 + 2, P["shroud_dark"])
        put(px, fx + 1, y0 + 2, P["shroud_dark"])
    for x in range(x0 + 1, x0 + w - 1, 2):
        put(px, x, y0 + h, P["gold"])
    put(px, x0 + w // 2, y0 + 1, P["led_green"])


def _bufo_body(variant="plain", look=(0, 1), bob=0, blink=False):
    px = blank()
    oy = bob
    # feet
    for fx in (5, 18):
        ellipse(px, fx + 0.5, 20.5 + oy, 2.6, 1.6, P["bufo_dark"])
    # body
    ellipse(px, 12, 14.5 + oy, 10.2, 6.8, P["bufo"], P["bufo_hi"], P["bufo_dark"])
    ellipse(px, 12, 17.2 + oy, 6.5, 3.4, P["bufo_belly"])
    # warts
    for wx, wy in ((5, 12), (19, 13), (8, 10), (16, 10), (4, 16), (20, 16)):
        put(px, wx, wy + oy, P["bufo_dark"])
    # eye bumps
    for ex in (6.5, 17.5):
        ellipse(px, ex, 8.5 + oy, 4.0, 3.6, P["bufo"], P["bufo_hi"], P["bufo_dark"])
    # eyes
    lx, ly = look
    for ex in (6, 17):
        if blink:
            rect(px, ex - 1, 8 + oy, ex + 2, 8 + oy, P["bufo_mouth"])
            continue
        rect(px, ex - 1, 7 + oy, ex + 2, 9 + oy, P["white"])
        put(px, ex - 1, 7 + oy, P["bufo"])
        put(px, ex + 2, 9 + oy, P["bufo"])
        px_, py_ = ex + (1 if lx > 0 else 0 if lx < 0 else (1 if ex < 12 else 0)), 8 + oy + (-1 if ly < 0 else 0 if ly == 0 else 0)
        rect(px, px_, py_, px_, py_ + 1, P["black"])
    # wide mouth (a smug curve)
    for x in range(4, 21):
        t = (x - 12) / 8.0
        y = 13 + oy + round(1.2 - 1.2 * t * t)
        put(px, x, y, P["bufo_mouth"])
    put(px, 3, 12 + oy, P["bufo_mouth"])
    put(px, 21, 12 + oy, P["bufo_mouth"])
    # nostrils
    put(px, 10, 11 + oy, P["bufo_dark"])
    put(px, 14, 11 + oy, P["bufo_dark"])
    if variant == "gpu":
        _gpu_mini(px, 7, 15 + oy, 10, 4)
        for hx in (6, 17):  # little hands holding it
            ellipse(px, hx + 0.5, 17 + oy, 1.6, 1.4, P["bufo_hi"])
    elif variant == "mayo":
        for mx, my in ((8, 13), (9, 14), (10, 14), (13, 14), (14, 14), (15, 13), (16, 13), (12, 15), (11, 15),
                       (5, 11), (6, 11), (19, 11), (18, 15), (17, 16), (9, 15)):
            put(px, mx, my + oy, P["mayo"])
        put(px, 12, 16 + oy, P["mayo_hi"])
        put(px, 17, 17 + oy, P["mayo_hi"])
    elif variant == "party":
        cone = blank()
        tri(cone, (12, 0.5), (8.2, 7.5), (15.8, 7.5), P["mistral_orange"])
        for y in range(8):
            for x in range(W):
                if cone[y][x][3] and (y in (2, 5)):
                    cone[y][x] = P["mistral_yellow"]
                elif cone[y][x][3] and y == 7:
                    cone[y][x] = P["mistral_red"]
        cone = shift(cone, 0, 1 + oy)
        paste(px, cone)
        put(px, 12, 1 + oy, P["white"])
        put(px, 11, 0 + oy, P["led_blue"])
        put(px, 13, 0 + oy, P["pink"])
    outline(px)
    ground_shadow(px, ry=1.2, cy=22.6)
    return px


def bufo(variant: str = "plain") -> list[str]:
    looks = {"up": (0, -1), "right": (1, 0), "down": (0, 1), "left": (-1, 0)}
    out = []
    for d in FACINGS:
        body = _bufo_body(variant, looks[d])
        out.append(save(f"bufo-{variant}-{d}", add_arrow(body, d)))
    return out


def bufo_frames(variant: str = "plain") -> list[str]:
    return [save(f"bufo-{variant}-idle0", _bufo_body(variant, (0, 1), 0)),
            save(f"bufo-{variant}-idle1", _bufo_body(variant, (0, 1), 1, blink=True))]


# ----------------------------------------------------------------------------------------------
# cats
# ----------------------------------------------------------------------------------------------
CAT_FUR = {
    "fat": ((255, 196, 110, 255), (238, 140, 46, 255), (184, 92, 30, 255), (255, 232, 196, 255)),
    "pixel": ((90, 90, 104, 255), (52, 52, 62, 255), (30, 30, 38, 255), (240, 240, 236, 255)),
    "sysadmin": ((196, 196, 204, 255), (150, 150, 162, 255), (98, 98, 112, 255), (226, 226, 230, 255)),
    "guard": ((200, 150, 100, 255), (156, 108, 66, 255), (104, 70, 42, 255), (236, 214, 180, 255)),
}


def _cat_body(variant="fat", look=(0, 1), leg=0, bob=0):
    hi, mid, dark, belly = CAT_FUR[variant]
    fat = variant == "fat"
    px = blank()
    oy = bob
    rx, ry, cy = (10.3, 8.0, 13.5) if fat else (8.0, 7.2, 13.5)
    # tail (behind, to the right)
    for i in range(6):
        put(px, 19 + (i > 2) + (i > 4), 15 - i + oy, dark)
        put(px, 20 + (i > 2) + (i > 4), 15 - i + oy, mid)
    # legs (tiny), walk offsets
    legs = [(7, 0), (10, 0), (14, 0), (17, 0)] if fat else [(8, 0), (11, 0), (13, 0), (16, 0)]
    for i, (lx, _) in enumerate(legs):
        up = (leg == 1 and i in (0, 3)) or (leg == 3 and i in (1, 2))
        rect(px, lx, 20 + oy - up, lx + 1, 21 + oy - up, dark)
    # ears
    for sx in (1, -1):
        ex = 12 - sx * (6 if fat else 5)
        tri(px, (ex - 3, 7.5 + oy), (ex + 3, 7.5 + oy), (ex - sx * 1.5, 1.5 + oy), mid)
        tri(px, (ex - 1.5, 7 + oy), (ex + 1.5, 7 + oy), (ex - sx * 1.0, 3.8 + oy), P["pink"])
    # round body + face
    ellipse(px, 12, cy + oy, rx, ry, mid, hi, dark)
    ellipse(px, 12, cy + 3.5 + oy, rx * 0.55, ry * 0.45, belly)
    if variant == "pixel":  # tuxedo blaze
        ellipse(px, 12, cy + 0.5 + oy, 2.5, 3.5, belly)
    if variant in ("fat", "sysadmin", "guard"):  # tabby stripes on the forehead
        for sx in (10, 12, 14):
            line(px, sx, 6 + oy + (sx == 12) - (1 if fat else 0), sx, 8 + oy, dark)
    # eyes
    lx, ly = look
    for ex in (8, 15):
        ey = 11 + oy
        rect(px, ex, ey, ex + 1, ey + 1, (250, 236, 120, 255) if variant != "pixel" else (140, 230, 140, 255))
        ppx = ex + (1 if lx > 0 else 0 if lx < 0 else (1 if ex < 12 else 0))
        ppy = ey + (0 if ly < 0 else 1 if ly > 0 else 0)
        put(px, ppx, ppy, P["black"])
        put(px, ppx, ppy - 1 + (ly <= 0), P["black"])
    # nose + mouth + whiskers
    put(px, 11, 13 + oy, P["pink"])
    put(px, 12, 13 + oy, P["pink"])
    put(px, 10, 14 + oy, dark)
    put(px, 13, 14 + oy, dark)
    put(px, 11, 15 + oy, dark)
    put(px, 12, 15 + oy, dark)
    for wy in (13, 15):
        line(px, 1 if fat else 3, wy + oy, 5, 14 + oy, P["white"])
        line(px, 18, 14 + oy, 22 if fat else 20, wy + oy, P["white"])
    if variant == "sysadmin":  # headset band + ear cups + mic
        for x in range(4, 20):
            t = (x - 12) / 8.0
            put(px, x, round(4 + 3 * t * t) + oy, P["black"])
        for ex in (3, 19):
            rect(px, ex, 9 + oy, ex + 2, 12 + oy, P["black"])
            put(px, ex + 1, 10 + oy, P["led_green"])
        line(px, 5, 12 + oy, 9, 16 + oy, P["black"])
        rect(px, 9, 16 + oy, 10, 16 + oy, P["mistral_orange"])
    elif variant == "guard":  # peaked cap
        rect(px, 6, 3 + oy, 17, 6 + oy, (40, 60, 120, 255))
        rect(px, 5, 7 + oy, 18, 7 + oy, (24, 30, 60, 255))
        put(px, 11, 4 + oy, P["gold"])
        put(px, 12, 4 + oy, P["gold"])
    elif variant == "fat":  # extra round belly highlight
        put(px, 9, 18 + oy, (255, 245, 225, 255))
    outline(px)
    ground_shadow(px, ry=1.2, cy=22.6, rx=9 if fat else 7)
    return px


def cat(variant: str = "fat") -> list[str]:
    looks = {"up": (0, -1), "right": (1, 0), "down": (0, 1), "left": (-1, 0)}
    return [save(f"cat-{variant}-{d}", add_arrow(_cat_body(variant, looks[d]), d)) for d in FACINGS]


def cat_walk(variant: str = "fat") -> list[str]:
    return [save(f"cat-{variant}-walk{i}", _cat_body(variant, (0, 1), leg=i, bob=(i % 2))) for i in range(4)]


# ----------------------------------------------------------------------------------------------
# mayo
# ----------------------------------------------------------------------------------------------
def _jar(shine=0):
    px = blank()
    # glass body
    rect(px, 5, 7, 18, 21, P["mayo"])
    rect(px, 4, 9, 19, 20, P["mayo"])
    rect(px, 5, 7, 6, 21, P["mayo_hi"])
    rect(px, 17, 8, 18, 21, P["mayo_dark"])
    rect(px, 19, 10, 19, 20, P["mayo_dark"])
    # lid
    rect(px, 5, 2, 18, 6, P["lid"])
    rect(px, 5, 2, 18, 2, P["lid_hi"])
    for x in range(6, 18, 2):
        put(px, x, 5, (28, 70, 160, 255))
    # label (mistral stripes) with a tiny egg
    for i, k in enumerate(("mistral_yellow", "mistral_amber", "mistral_orange", "mistral_redorange", "mistral_red")):
        rect(px, 4, 11 + i, 19, 11 + i, P[k])
    ellipse(px, 12, 13.2, 2.6, 2.4, P["white"])
    rect(px, 11, 13, 12, 13, P["mistral_amber"])
    # shine streak
    sx = [8, 13][shine % 2]
    for i in range(4):
        put(px, sx + i // 2, 17 + i - (i // 2) * 0, P["white"])
    if shine:
        put(px, 15, 8, P["white"])
        put(px, 14, 9, P["white"])
        put(px, 16, 9, P["white"])
        put(px, 15, 10, P["white"])
    outline(px)
    ground_shadow(px, ry=1.0, cy=22.8, rx=8)
    return px


def mayo_jar() -> str:
    return save("mayo-jar", _jar(0))


def mayo_jar_frames() -> list[str]:
    return [save("mayo-jar-shine0", _jar(0)), save("mayo-jar-shine1", _jar(1))]


def mayo_splat() -> str:
    px = blank()
    for cx, cy, r in ((12, 13, 5.5), (7, 10, 2.6), (17, 16, 2.8), (16, 8, 2.0), (6, 17, 1.8), (19, 11, 1.4)):
        ellipse(px, cx, cy, r, r * 0.8, P["mayo"], P["mayo_hi"], P["mayo_dark"])
    put(px, 3, 8, P["mayo"])
    put(px, 21, 18, P["mayo"])
    put(px, 10, 20, P["mayo"])
    outline(px, P["mayo_dark"])
    return save("mayo-splat", px)


# ----------------------------------------------------------------------------------------------
# GPU / fire / racks
# ----------------------------------------------------------------------------------------------
def _gpu(state="cool"):
    px = blank()
    tint = {"cool": None, "warm": (255, 140, 40), "hot": (255, 50, 30), "dead": (110, 110, 110)}[state]
    sh, shi, shd = P["shroud"], P["shroud_hi"], P["shroud_dark"]
    if state == "dead":
        sh, shi, shd = (70, 70, 74, 255), (104, 104, 108, 255), (44, 44, 48, 255)
    # PCB edge + gold fingers
    rect(px, 2, 17, 20, 18, (30, 60, 40, 255))
    for x in range(4, 19, 2):
        rect(px, x, 19, x, 20, P["gold"] if state != "dead" else P["gold_dark"])
    # shroud
    rect(px, 1, 5, 22, 17, sh)
    rect(px, 1, 5, 22, 5, shi)
    rect(px, 1, 5, 1, 17, shi)
    rect(px, 22, 6, 22, 17, shd)
    rect(px, 1, 17, 22, 17, shd)
    # gold trim stripe along the bottom of the shroud
    rect(px, 2, 16, 21, 16, P["gold"] if state != "dead" else P["gold_dark"])
    # bracket
    rect(px, 0, 4, 0, 19, P["steel_hi"])
    # fans
    for fx in (7, 16):
        ellipse(px, fx, 11, 4.3, 4.3, shd)
        ellipse(px, fx, 11, 3.4, 3.4, P["steel_dark"] if state != "dead" else (60, 60, 64, 255))
        for a in range(4):
            ang = a * math.pi / 2 + (0.4 if fx == 7 else 1.2)
            line(px, fx, 11, fx + round(3 * math.cos(ang)), 11 + round(3 * math.sin(ang)),
                 P["steel_hi"] if state != "dead" else (90, 90, 94, 255))
        put(px, fx, 11, shi)
    # LED strip on the top edge
    led = {"cool": P["led_green"], "warm": P["led_amber"], "hot": P["led_red"], "dead": (60, 60, 60, 255)}[state]
    rect(px, 10, 6, 13, 6, led)
    rect(px, 3, 15, 5, 15, P["gold"] if state != "dead" else P["steel_dark"])  # tiny badge
    if tint and state != "dead":
        for y in range(H):
            for x in range(W):
                p = px[y][x]
                if p[3]:
                    px[y][x] = _mix(p, tint + (255,), 0.25 if state == "warm" else 0.4)
    outline(px)
    if state == "hot":  # heat shimmer above
        for x in (5, 11, 17):
            for i, y in enumerate((3, 2, 1)):
                put(px, x + (i % 2), y, P["flame_o"] if i < 2 else P["flame_y"])
    if state == "dead":  # crack + smoke puffs
        line(px, 9, 6, 13, 12, P["black"])
        line(px, 13, 12, 11, 16, P["black"])
        for cx, cy, r in ((6, 2.5, 1.6), (9, 1.5, 1.3)):
            ellipse(px, cx, cy, r, r, (150, 150, 156, 200))
    return px


def gpu(state: str = "cool") -> str:
    assert state in ("cool", "warm", "hot", "dead"), state
    return save(f"gpu-{state}", _gpu(state))


def _flame(i):
    px = blank()
    sway = [0, 1, -1][i % 3]
    tall = [0, 2, 1][i % 3]
    ellipse(px, 12, 16 - tall / 2, 8, 7 + tall / 2, P["flame_r"])
    tri(px, (5, 14), (19, 14), (12 + sway * 2, 2 - tall), P["flame_r"])
    tri(px, (6, 12), (11, 12), (7 - sway, 4 - tall + (i == 1) * 2), P["flame_r"])
    tri(px, (13, 12), (19, 12), (17 + sway, 5 - tall), P["flame_r"])
    ellipse(px, 12, 17 - tall / 3, 6, 5, P["flame_o"])
    tri(px, (7, 15), (17, 15), (12 + sway, 6 - tall), P["flame_o"])
    ellipse(px, 12, 18, 3.5, 3, P["flame_y"])
    tri(px, (9.5, 17), (14.5, 17), (12 - sway, 11 - tall), P["flame_y"])
    outline(px, (120, 20, 10, 255))
    # sparks
    for sx, sy in [((4, 3), (20, 6)), ((18, 2), (3, 8)), ((6, 6), (19, 1))][i % 3]:
        pass
    sp = [((4, 3), (20, 6)), ((18, 2), (3, 8)), ((6, 6), (20, 1))][i % 3]
    for sx, sy in sp:
        put(px, sx, sy, P["flame_y"])
    return px


def gpu_fire_frames() -> list[str]:
    return [save(f"fire-{i}", _flame(i)) for i in range(3)]


fire_frames = gpu_fire_frames


def _rack(state="idle", frame=0):
    px = blank()
    rect(px, 4, 0, 19, 22, P["steel_dark"])
    rect(px, 4, 0, 19, 0, P["steel"])
    rect(px, 4, 0, 4, 22, P["steel"])
    rect(px, 19, 1, 19, 22, (44, 46, 56, 255))
    rect(px, 5, 22, 18, 22, P["black"])
    for u in range(6):
        y = 2 + u * 3 + (u > 0) * 0
        if state == "empty":
            rect(px, 6, y, 17, y + 1, (22, 22, 28, 255))
            put(px, 6, y, P["steel"])
            put(px, 17, y, P["steel"])
            continue
        rect(px, 6, y, 17, y + 1, P["shroud"])
        rect(px, 6, y, 17, y, P["shroud_hi"])
        for x in range(12, 17, 2):  # vents
            put(px, x, y + 1, P["shroud_dark"])
        if state == "idle":
            leds = [P["led_green"], P["shroud_dark"], P["shroud_dark"]]
            if u % 2:
                leds = [P["led_green"], P["led_green"], P["shroud_dark"]]
        else:
            on = [(u + frame) % 2 == 0, (u * 3 + frame) % 3 != 0, (u + frame * 2) % 4 < 2]
            cols = [P["led_green"], P["led_amber"], P["led_blue"]]
            leds = [cols[k] if on[k] else P["shroud_dark"] for k in range(3)]
        for k, c in enumerate(leds):
            put(px, 7 + k * 2, y + 1, c)
    outline(px)
    return px


def server_rack(state: str = "idle") -> str:
    assert state in ("idle", "busy", "empty"), state
    return save(f"rack-{state}", _rack(state, 0))


def rack_frames() -> list[str]:
    return [save(f"rack-busy-{i}", _rack("busy", i)) for i in range(2)]


# ----------------------------------------------------------------------------------------------
# Mistral-style logo
# ----------------------------------------------------------------------------------------------
STRIPES = ["mistral_yellow", "mistral_amber", "mistral_orange", "mistral_redorange", "mistral_red"]
M_GRID = ["X...X",
          "XX.XX",
          "XXXXX",
          "X.X.X",
          "X...X"]


def _logo(scale_x=1.0, shine=-1, back=False):
    """Blocky M of 4x4 squares; each row one stripe colour, bevelled squares, black drop shadow.
    scale_x squashes it horizontally around the centre (spin frames); back=True is the darker reverse."""
    src = blank()
    cell, ox, oy = 4, 2, 1
    for r, row in enumerate(M_GRID):
        for c, ch in enumerate(row):
            if ch != "X":
                continue
            base = P[STRIPES[r]]
            if back:
                base = _mix(base, (60, 20, 10), 0.45)
            for y in range(cell):
                for x in range(cell):
                    col = base
                    if x == cell - 1 or y == cell - 1:
                        col = _mix(base, (90, 20, 0), 0.3)
                    elif x == 0 and y == 0:
                        col = _mix(base, (255, 255, 255), 0.5)
                    put(src, ox + c * cell + x, oy + r * cell + y, col)
    for r, row in enumerate(M_GRID):  # 1px black drop shadow (down-right) behind the blocks
        for c, ch in enumerate(row):
            if ch == "X":
                for y in range(cell):
                    for x in range(cell):
                        X, Y = ox + c * cell + x + 1, oy + r * cell + y + 1
                        if 0 <= X < W and 0 <= Y < H and src[Y][X][3] == 0:
                            src[Y][X] = P["black"]
    px = blank()
    cx = 12.0
    for y in range(H):
        for x in range(W):
            u = int((x + 0.5 - cx) / max(scale_x, 0.01) + cx)
            if 0 <= u < W:
                px[y][x] = src[y][u]
    if shine >= 0:  # diagonal glint
        for y in range(H):
            for x in range(W):
                if px[y][x][3] and px[y][x] != P["black"] and abs((x + y) - shine) <= 1:
                    px[y][x] = _mix(px[y][x], (255, 255, 255), 0.6)
    outline(px)
    return px


def mistral_logo() -> str:
    return save("mistral-logo", _logo())


def mistral_logo_spin() -> list[str]:
    return [save("mistral-spin-0", _logo(1.0, shine=14)), save("mistral-spin-1", _logo(0.55)),
            save("mistral-spin-2", _logo(0.15)), save("mistral-spin-3", _logo(0.55, back=True))]


# ----------------------------------------------------------------------------------------------
# items
# ----------------------------------------------------------------------------------------------
def envelope() -> str:
    px = blank()
    rect(px, 2, 6, 21, 19, (244, 238, 222, 255))
    rect(px, 2, 19, 21, 19, (206, 196, 172, 255))
    for x in range(2, 22):  # flap V
        y = 6 + round(abs(x - 11.5) * -0.62 + 7)
        put(px, x, y, (196, 184, 160, 255))
        for yy in range(6, y):
            if (x + yy) % 7 == 0:
                pass
    line(px, 2, 19, 9, 13, (206, 196, 172, 255))
    line(px, 21, 19, 14, 13, (206, 196, 172, 255))
    ellipse(px, 12, 13, 2.2, 2.2, P["mistral_red"], P["mistral_orange"])
    outline(px)
    ground_shadow(px, ry=1.0, cy=21.5, rx=9)
    return save("envelope", px)


def coolant() -> str:
    px = blank()
    rect(px, 5, 7, 18, 21, P["coolant"])
    rect(px, 5, 7, 7, 21, P["coolant_hi"])
    rect(px, 16, 7, 18, 21, (30, 140, 180, 255))
    rect(px, 9, 3, 14, 6, P["steel"])
    rect(px, 9, 3, 14, 3, P["steel_hi"])
    rect(px, 15, 9, 20, 10, P["steel_dark"])  # handle
    rect(px, 19, 9, 20, 15, P["steel_dark"])
    # snowflake
    for d in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1), (2, 0), (-2, 0), (0, 2), (0, -2), (1, 1), (-1, -1),
              (1, -1), (-1, 1)):
        put(px, 11 + d[0], 14 + d[1], P["white"])
    outline(px)
    ground_shadow(px, ry=1.0, cy=22.8, rx=8)
    return save("coolant", px)


def cable() -> str:
    px = blank()
    # coil
    for r in (7.0, 5.0, 3.0):
        for a in range(0, 360, 4):
            t = math.radians(a)
            put(px, 11 + r * math.cos(t), 12 + r * 0.8 * math.sin(t), (40, 40, 50, 255))
    for r in (6.0, 4.0):
        for a in range(0, 360, 4):
            t = math.radians(a)
            put(px, 11 + r * math.cos(t), 12 + r * 0.8 * math.sin(t), (80, 170, 255, 255))
    # tail + plug
    line(px, 17, 14, 20, 18, (80, 170, 255, 255))
    rect(px, 19, 18, 22, 21, P["steel"])
    put(px, 20, 22, P["gold"])
    put(px, 22, 22, P["gold"])
    outline(px)
    return save("cable", px)


# ----------------------------------------------------------------------------------------------
# floors / walls / canvas
# ----------------------------------------------------------------------------------------------
def datacenter_floor() -> str:
    px = blank(P["floor"])
    rect(px, 0, 0, 23, 0, P["floor_hi"])
    rect(px, 0, 0, 0, 23, P["floor_hi"])
    rect(px, 0, 23, 23, 23, P["floor_dark"])
    rect(px, 23, 0, 23, 23, P["floor_dark"])
    for y in range(3, 22, 3):  # perforation grid
        for x in range(3, 22, 3):
            put(px, x, y, P["floor_dark"])
    for x, y in ((1, 1), (22, 1), (1, 22), (22, 22)):
        put(px, x, y, P["steel_dark"])
    return save("dcfloor", px)


def _wall16(idx):
    L, R, U, D = idx & 1, idx & 2, idx & 4, idx & 8
    px = blank(P["wall"])
    # panel texture
    for y in range(4, 24, 8):
        rect(px, 0, y, 23, y, P["wall_dark"])
    for x in (6, 17):
        for y in range(0, 24):
            if (y // 8) % 2 == (x == 17):
                put(px, x, y, P["wall_dark"])
    # small status lights
    put(px, 11, 8, P["led_blue"])
    put(px, 12, 16, P["led_green"])
    # bevels on open sides
    if not U:
        rect(px, 0, 0, 23, 2, P["wall_hi"])
        rect(px, 0, 0, 23, 0, P["outline"])
        rect(px, 0, 1, 23, 1, (130, 138, 160, 255))
    if not D:
        rect(px, 0, 20, 23, 23, P["wall_dark"])
        for x in range(0, 24, 4):  # hazard trim
            rect(px, x, 21, x + 1, 22, P["mistral_amber"])
        rect(px, 0, 23, 23, 23, P["outline"])
    if not L:
        rect(px, 0, 0, 0, 23, P["outline"])
        rect(px, 1, 0, 1, 23, P["wall_hi"])
    if not R:
        rect(px, 23, 0, 23, 23, P["outline"])
        rect(px, 22, 0, 22, 23, P["wall_dark"])
    return px


def datacenter_wall16() -> list[str]:
    return [save(f"dcwall-{i}", _wall16(i)) for i in range(16)]


def pixel_canvas(color="white") -> str:
    c = _rgba(color)
    name = color if isinstance(color, str) and not color.startswith("#") else "x" + "".join(f"{v:02x}" for v in c[:3])
    px = blank(c)
    lo, hi_ = _mix(c, (0, 0, 0), 0.18), _mix(c, (255, 255, 255), 0.18)
    rect(px, 0, 0, 23, 0, hi_)
    rect(px, 0, 0, 0, 23, hi_)
    rect(px, 0, 23, 23, 23, lo)
    rect(px, 23, 0, 23, 23, lo)
    put(px, 5, 5, hi_)
    put(px, 6, 5, hi_)
    put(px, 5, 6, hi_)
    return save(f"canvas-{name}", px)


# ----------------------------------------------------------------------------------------------
# animation
# ----------------------------------------------------------------------------------------------
def animate(obj_name: str, n_frames: int, period: int, start_tile: int = 0, over=()) -> tuple[str, str]:
    """(actions_yaml, initial_actions_yaml) cycling render tiles start_tile..start_tile+n-1 every
    `period` ticks. See module docstring for usage + quirks."""
    assert n_frames >= 1 and period >= 1
    act = f"{obj_name}_anim"
    inputs = "\n".join(
        f"        {k}: {{Description: frame {k - 1}, VectorToDest: [0, 0], "
        f"MetaData: {{tile: {start_tile + k - 1}, next: {k % n_frames + 1}}}}}" for k in range(1, n_frames + 1))
    dst = "[" + ", ".join([obj_name, *over]) + "]"
    actions = (f"  - Name: {act}\n    InputMapping:\n      Internal: true\n      Inputs:\n{inputs}\n"
               f"    Behaviours:\n"
               f"      - Src:\n          Object: {obj_name}\n          Commands:\n"
               f"            - set_tile: meta.tile\n"
               f"            - exec: {{Action: {act}, ActionId: meta.next, Delay: {period}}}\n"
               f"        Dst: {{Object: {dst}}}\n")
    initial = f"    InitialActions:\n      - Action: {act}\n        ActionId: 1\n"
    return actions, initial


# ----------------------------------------------------------------------------------------------
# everything / sheet / validation
# ----------------------------------------------------------------------------------------------
BUFO_VARIANTS = ("plain", "gpu", "mayo", "party")
CAT_VARIANTS = ("fat", "pixel", "sysadmin", "guard")


def build_all() -> list[str]:
    paths = []
    for v in BUFO_VARIANTS:
        paths += bufo(v) + bufo_frames(v)
    for v in CAT_VARIANTS:
        paths += cat(v) + cat_walk(v)
    paths += [mayo_jar(), *mayo_jar_frames(), mayo_splat()]
    paths += [gpu(s) for s in ("cool", "warm", "hot", "dead")] + gpu_fire_frames()
    paths += [server_rack(s) for s in ("idle", "busy", "empty")] + rack_frames()
    paths += [mistral_logo(), *mistral_logo_spin(), envelope(), coolant(), cable(), datacenter_floor()]
    paths += datacenter_wall16()
    paths += [pixel_canvas(c) for c in CANVAS_COLORS]
    return paths


def contact_sheet(scale: int = 4, cols: int = 12) -> str:
    """Every sprite at `scale`x on a checkerboard, row-major in build order; index in _contact_sheet.txt."""
    paths = build_all()
    cell = W * scale + 6
    rows = (len(paths) + cols - 1) // cols
    sw, sh = cols * cell + 6, rows * cell + 6
    sheet = [[(48, 44, 56, 255)] * sw for _ in range(sh)]
    for i, rel in enumerate(paths):
        px = _written[rel]
        ox, oy = 6 + (i % cols) * cell, 6 + (i // cols) * cell
        for y in range(H * scale):
            for x in range(W * scale):
                bg = (200, 200, 208) if ((x // 8 + y // 8) % 2) else (170, 170, 180)
                p = px[y // scale][x // scale]
                a = p[3] / 255
                sheet[oy + y][ox + x] = tuple(int(p[k] * a + bg[k] * (1 - a)) for k in range(3)) + (255,)
    out = OUT_DIR / "_contact_sheet.png"
    C.save_png(out, sw, sh, sheet)
    (OUT_DIR / "_contact_sheet.txt").write_text(
        "\n".join(f"r{i // cols} c{i % cols}: {p}" for i, p in enumerate(paths)) + "\n")
    return f"{DIR}/_contact_sheet.png"


def validate() -> int:
    n = 0
    for rel in _written:
        w, h, px = C.load_png(C.IMAGES_DIR / rel)
        assert (w, h) == (W, H), f"{rel}: {w}x{h}"
        assert px == [list(map(tuple, r)) for r in _written[rel]], f"{rel}: round-trip mismatch"
        n += 1
    return n


if __name__ == "__main__":
    sheet = contact_sheet()
    print(f"{len(_written)} sprites ok (round-trip {validate()}), sheet: resources/images/{sheet}")
