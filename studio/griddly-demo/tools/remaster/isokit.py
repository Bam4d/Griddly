"""Helpers for the isometric remastered games (slimefarm, minecrawler).

These games are drawn mainly by ISOMETRIC observers but still work top-down, so every object carries
four sprite sets: Sprite2D (+ FollowCam/CloseUp aliases), Isometric (+ IsoFollow alias) and Block2D.

  * header()       cameras: Sprite2D (global), FollowCam 15x11, CloseUp 9x9 (SPRITE_2D, track avatar),
                   Isometric (global iso, whole map) and IsoFollow (ISOMETRIC, tracks the avatar).
  * obj()          object definition with a 2D sprite and an iso sprite (each a list of render tiles).
  * terrain()      the shared terrain objects of common (same names/chars) with iso sprites added.
  * image helpers  derive extra 32x48 iso sprites from existing art (tint, lowered blocks, billboards
                   of 2D sprites, stratega 256px tiles) and write them to resources/images/remastered/.

Iso tile geometry (oryx_iso_dungeon, 32x48): the floor diamond spans rows 28..47; walls are cubes whose
top face spans rows 4..20. IsoTileHeight 16 => each grid step moves 8 px down the screen, so tall
cubes hide the cells behind them; lowered blocks (block()) keep tunnels visible.
"""
from __future__ import annotations

import colorsys
import json
from pathlib import Path

import common as C

ISO = "oryx/oryx_iso_dungeon"
CAMS_2D = ["FollowCam", "CloseUp"]
CAMS_ISO = ["IsoFollow"]
ISO_FOLLOW = (11, 11)

_written: set[str] = set()


# ----------------------------------------------------------------------------------------------
# image helpers (all return an image path relative to resources/images)
# ----------------------------------------------------------------------------------------------
def _load(src: str):
    return C.load_png(C.IMAGES_DIR / src)


def _save(rel: str, w, h, px) -> str:
    C.save_png(C.IMAGES_DIR / rel, w, h, px)
    _written.add(rel)
    return rel


def blank(w=32, h=48):
    return [[(0, 0, 0, 0)] * w for _ in range(h)]


def tint(src: str, out: str, hue: float | None = None, sat: float = 1.0, val: float = 1.0,
         rgb: tuple[float, float, float] | None = None) -> str:
    """Recolour: rotate hue to `hue` (0..1) / scale saturation and value, or multiply by `rgb`."""
    w, h, px = _load(src)
    res = []
    for row in px:
        r2 = []
        for r, g, b, a in row:
            if a == 0:
                r2.append((0, 0, 0, 0))
                continue
            if rgb:
                nr, ng, nb = r * rgb[0], g * rgb[1], b * rgb[2]
            else:
                hh, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
                if hue is not None:
                    hh = hue
                nr, ng, nb = (c * 255 for c in colorsys.hsv_to_rgb(hh, min(1, s * sat), min(1, v * val)))
            r2.append((min(255, int(nr)), min(255, int(ng)), min(255, int(nb)), a))
        res.append(r2)
    return _save(out, w, h, res)


def lowered(src: str, out: str, by: int, split: int = 20) -> str:
    """A cube sprite with `by` rows of its side faces removed (a lower block): rows above `split` move
    down by `by`, the base stays on the floor."""
    w, h, px = _load(src)
    res = blank(w, h)
    for y in range(h):
        if y >= split + by:
            res[y] = list(px[y])
        elif y - by >= 0:
            res[y] = list(px[y - by])
    return _save(out, w, h, res)


def overlay(base: str, out: str, draw) -> str:
    """Copy `base` and let draw(px) paint on it (px rows of RGBA tuples, mutable)."""
    w, h, px = _load(base)
    px = [list(r) for r in px]
    draw(px)
    return _save(out, w, h, px)


def billboard(src: str, out: str, size: int = 20, foot: int = 40, dx: int = 0) -> str:
    """Stand a 2D sprite up in an iso cell: nearest-neighbour scaled to `size` px wide, centred, its
    bottom on row `foot` (the floor diamond's centre is row ~38)."""
    w, h, px = _load(src)
    # crop transparent margins
    ys = [y for y in range(h) for x in range(w) if px[y][x][3] > 0]
    xs = [x for y in range(h) for x in range(w) if px[y][x][3] > 0]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    cw, ch = x1 - x0 + 1, y1 - y0 + 1
    s = size / max(cw, ch)
    ow, oh = max(1, round(cw * s)), max(1, round(ch * s))
    res = blank()
    left = 16 - ow // 2 + dx
    top = foot - oh
    for y in range(oh):
        for x in range(ow):
            p = px[y0 + min(ch - 1, int(y / s))][x0 + min(cw - 1, int(x / s))]
            if p[3] and 0 <= top + y < 48 and 0 <= left + x < 32:
                res[top + y][left + x] = p
    return _save(out, 32, 48, res)


def stratega(src: str, out: str) -> str:
    """A 256x256 stratega iso tile as a 32x48 oryx-style iso tile (box downscale, bottom aligned)."""
    w, h, px = _load(src)
    f = w // 32
    res = blank()
    for oy in range(h // f):
        for ox in range(32):
            acc = [0, 0, 0, 0]
            for y in range(oy * f, oy * f + f):
                for x in range(ox * f, ox * f + f):
                    r, g, b, a = px[y][x]
                    acc[0] += r * a
                    acc[1] += g * a
                    acc[2] += b * a
                    acc[3] += a
            if acc[3] == 0:
                continue
            a = acc[3] / (f * f)
            if a < 40:
                continue
            res[oy + 48 - h // f][ox] = (int(acc[0] / acc[3]), int(acc[1] / acc[3]), int(acc[2] / acc[3]), 255)
    return _save(out, 32, 48, res)


def sphere(out: str, base: tuple[int, int, int], cx=16, cy=33, r=10.5, squash=0.85, w=32, h=48,
           shadow=True) -> str:
    """A shaded boulder (sitting on the iso floor diamond by default; w=h=24, shadow=False for 2D)."""
    res = blank(w, h)
    lx, ly, lz = -0.5, -0.6, 0.62
    for y in range(h):
        for x in range(w):
            dx, dy = (x + 0.5 - cx) / r, (y + 0.5 - cy) / (r * squash)
            d = dx * dx + dy * dy
            if d > 1:
                continue
            nz = (1 - d) ** 0.5
            lum = max(0.0, dx * lx + dy * ly + nz * lz)
            k = 0.35 + 0.85 * lum
            # a few speckles for texture
            if ((x * 7 + y * 13) % 11) == 0:
                k *= 0.8
            edge = d > 0.86
            if edge:
                k *= 0.45
            res[y][x] = tuple(min(255, int(c * k)) for c in base) + (255,)
    if not shadow:
        return _save(out, w, h, res)
    # contact shadow
    for y in range(40, 45):
        for x in range(32):
            if res[y][x][3] == 0 and ((x + 0.5 - 16) / 12) ** 2 + ((y + 0.5 - 42) / 3) ** 2 <= 1:
                res[y][x] = (20, 16, 14, 150)
    return _save(out, w, h, res)


def beam(out: str) -> str:
    """Timber mine support: two posts and a lintel standing in the cell."""
    res = blank()
    wood, dark, light = (139, 94, 52), (82, 52, 28), (178, 128, 76)

    def rect(x0, y0, x1, y1, col):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                res[y][x] = col + (255,)
    for px_ in (6, 23):
        rect(px_, 14, px_ + 2, 41, wood)
        rect(px_ + 3, 14, px_ + 3, 41, dark)
        rect(px_, 14, px_, 41, light)
    rect(4, 11, 27, 14, wood)
    rect(4, 15, 27, 15, dark)
    rect(4, 11, 27, 11, light)
    for x in (9, 17, 22):
        res[12][x] = dark + (255,)
    return _save(out, 32, 48, res)


def put(px, x, y, col):
    if 0 <= y < len(px) and 0 <= x < len(px[0]):
        px[y][x] = col if len(col) == 4 else col + (255,)


# ----------------------------------------------------------------------------------------------
# GDY builders
# ----------------------------------------------------------------------------------------------
def sprites(entries: list) -> str:
    """Render-tile list. Each entry: an image path, a (path, {extra keys}) tuple, or raw YAML text
    starting with '-' (e.g. common.tiled())."""
    out = []
    for e in entries:
        if isinstance(e, str) and e.lstrip().startswith("-"):
            out.append(e)
            continue
        path, extra = (e, {}) if isinstance(e, str) else e
        line = f"        - Image: {path}"
        for k, v in extra.items():
            line += f"\n          {k}: {json.dumps(v) if not isinstance(v, str) else v}"
        out.append(line)
    return "\n".join(out)


def obj(name: str, char: str | None, z: int, sprite2d: list, iso: list, color: str = "[0.6, 0.6, 0.6]",
        shape: str = "square", scale: float = 0.9, extra: str = "", block_scales: list | None = None) -> str:
    """Object with Sprite2D (shared by FollowCam/CloseUp), Isometric (shared by IsoFollow) and Block2D.
    sprite2d / iso: lists of render tiles (same length; see sprites())."""
    assert len(sprite2d) == len(iso), f"{name}: {len(sprite2d)} 2D tiles vs {len(iso)} iso tiles"
    n = len(sprite2d)
    scales = block_scales or [scale] * n
    block = "\n".join(f"        - Shape: {shape}\n          Color: {color}\n          Scale: {s}" for s in scales)
    mc = f"    MapCharacter: {json.dumps(char)}\n" if char else ""
    s2 = "".join(f"      {c}: *{name}_sprite\n" for c in CAMS_2D)
    si = "".join(f"      {c}: *{name}_iso\n" for c in CAMS_ISO)
    return (f"  - Name: {name}\n{mc}    Z: {z}\n{extra}    Observers:\n"
            f"      Sprite2D: &{name}_sprite\n{sprites(sprite2d)}\n{s2}"
            f"      Isometric: &{name}_iso\n{sprites(iso)}\n{si}"
            f"      Block2D:\n{block}\n")


def iso_floor(path: str) -> tuple:
    return (path, {"TilingMode": "ISO_FLOOR"})


def terrain(include: set[str] | None = None) -> str:
    """common's terrain objects (same names, chars and Z) with Isometric sprites. All ground objects are
    always included (EMPTY_OR_GROUND names them); scenery only if listed in `include` (None = all)."""
    floor = f"{ISO}/floor-1.png"
    dirt = tint(floor, "remastered/iso-dirt.png", rgb=(1.25, 0.9, 0.55))
    sand = tint(floor, "remastered/iso-sand.png", rgb=(1.75, 1.45, 0.85))
    bridge = tint(floor, "remastered/iso-bridge.png", rgb=(1.3, 0.85, 0.45))
    rock = lowered(tint(f"{ISO}/wall-grey-1.png", "remastered/iso-rock-full.png", rgb=(1.15, 0.9, 0.7)),
                   "remastered/iso-rock.png", 10)
    defs = {
        "grass": (C.tiled("gvgai/oryx/grass", "_"), iso_floor(f"{ISO}/grass-1.png"), "[0.25, 0.55, 0.25]"),
        "moss": (C.TG + "floor_moss_a.png", iso_floor(f"{ISO}/floor-2.png"), "[0.25, 0.4, 0.2]"),
        "dirt": (C.TG + "floor_dirt_a.png", iso_floor(dirt), "[0.3, 0.22, 0.15]"),
        "sand": (C.TG + "floor_sand_a.png", iso_floor(sand), "[0.7, 0.55, 0.3]"),
        "bones": (C.TG + "bones_a.png", billboard(C.TG + "bones_a.png", "remastered/iso-bones.png", 14, 41),
                  "[0.8, 0.8, 0.75]"),
        "mushroom": ("gvgai/oryx/mushroom2.png",
                     billboard("gvgai/oryx/mushroom2.png", "remastered/iso-mushroom.png", 12, 40), "[0.8, 0.2, 0.2]"),
        "skull": ("gvgai/oryx/skull1.png", billboard("gvgai/oryx/skull1.png", "remastered/iso-skull.png", 11, 40),
                  "[0.9, 0.9, 0.9]"),
        "bridge": ("gvgai/newset/logm.png", iso_floor(bridge), "[0.55, 0.4, 0.25]"),
        "water": (C.TG + "water.png", (f"{ISO}/water-1.png", {"TilingMode": "ISO_FLOOR", "Offset": [0, 4]}),
                  "[0.1, 0.35, 0.7]"),
        "tree": ("gvgai/oryx/tree2.png", stratega("stratega/forest.png", "remastered/iso-tree.png"), "[0.05, 0.4, 0.2]"),
        "bush": ("gvgai/oryx/bush1.png", f"{ISO}/bush-1.png", "[0.1, 0.45, 0.15]"),
        "rock": (C.tiled("gvgai/oryx/dirtWall", "_"), rock, "[0.45, 0.25, 0.12]"),
        "hedge": (C.tiled("oryx/oryx_fantasy/wall9"), lowered(f"{ISO}/wall-green-1.png", "remastered/iso-hedge.png", 12),
                  "[0.15, 0.35, 0.15]"),
        "stone": (C.tiled("oryx/oryx_fantasy/wall2"), f"{ISO}/wall-grey-1.png", "[0.45, 0.45, 0.45]"),
        "tombstone": ("gvgai/oryx/tombstone1.png",
                      billboard("gvgai/oryx/tombstone1.png", "remastered/iso-tombstone.png", 18, 41), "[0.5, 0.5, 0.5]"),
        "torch": ("gvgai/oryx/torch1.png", billboard("gvgai/oryx/torch1.png", "remastered/iso-torch.png", 18, 40),
                  "[1.0, 0.6, 0.1]"),
    }
    out = []
    for name in C.GROUND_NAMES + C.SCENERY_NAMES:
        if name in C.SCENERY_CHARS and include is not None and name not in include:
            continue
        s2, si, color = defs[name]
        char = C.GROUND_CHARS.get(name) or C.SCENERY_CHARS[name]
        z = 0 if name in C.GROUND_CHARS else 1
        out.append(obj(name, char, z, [s2], [si], color, "square", 0.9 if z == 0 else 1.0))
    return "".join(out)


def header(name: str, description: str, avatar: str, win: list[str], lose: list[str], background: str,
           iso_background: str, follow=(15, 11), closeup=(9, 9), iso_follow=ISO_FOLLOW, variables: str = "") -> str:
    """Like common.header() plus the isometric cameras. win / lose: condition lines (YAML after '- ')."""
    bg = C.BACKGROUNDS.get(background, background)
    desc = "\n".join("    " + line for line in description.strip().splitlines())
    vars_block = f"  Variables:\n{variables}" if variables else ""
    win_block = "".join(f"      - {w}\n" for w in win)
    lose_block = "".join(f"      - {lc}\n" for lc in lose)
    lose_section = f"    Lose:\n{lose_block}" if lose else ""
    cam = lambda w, h: (f"      Type: SPRITE_2D\n      TileSize: 24\n      BackgroundTile: {bg}\n"
                        f"      RotateAvatarImage: false\n      RotateWithAvatar: false\n      TrackAvatar: true\n"
                        f"      Width: {w}\n      Height: {h}\n")
    iso = (f"      TileSize: [32, 48]\n      IsoTileHeight: 16\n      IsoTileDepth: 4\n"
           f"      BackgroundTile: {iso_background}\n")
    return f'''Version: "0.1"
Environment:
  Name: {name}
  Description: |
{desc}
    Generated by studio/griddly-demo/tools/remaster — edit the generator, not this file.
  # Cameras are observers. The demo picks one by name and the engine renders it.
  #   Isometric  whole level, isometric (the main view of this game)
  #   IsoFollow  {iso_follow[0]}x{iso_follow[1]} isometric window that follows the avatar
  #   Sprite2D   whole level, top-down
  #   FollowCam  {follow[0]}x{follow[1]} top-down window that follows the avatar
  #   CloseUp    {closeup[0]}x{closeup[1]} top-down window that follows the avatar (also the agent's default view)
  Observers:
    Isometric:
{iso}    IsoFollow:
      Type: ISOMETRIC
{iso}      TrackAvatar: true
      RotateWithAvatar: false
      Width: {iso_follow[0]}
      Height: {iso_follow[1]}
    Sprite2D:
      TileSize: 24
      BackgroundTile: {bg}
      RotateAvatarImage: false
      TrackAvatar: false
    FollowCam:
{cam(*follow)}    CloseUp:
{cam(*closeup)}    Block2D:
      TileSize: 24
{vars_block}  Player:
    Observer:
      TrackAvatar: true
      RotateWithAvatar: false
      RotateAvatarImage: false
      Width: {closeup[0]}
      Height: {closeup[1]}
    AvatarObject: {avatar}
  Termination:
    Win:
{win_block}{lose_section}  Levels:
'''


def written() -> list[str]:
    """Images written by this module so far (relative to resources/images)."""
    return sorted(_written)


def fit(src: str, out: str, w: int = 24, h: int = 24, size: int = 20, foot: int | None = None, dx: int = 0,
        hue: float | None = None, sat: float = 1.0, val: float = 1.0) -> str:
    """Crop `src` to its opaque pixels, scale (nearest) so the longer side is `size`, centre it in a
    w x h canvas with its bottom on row `foot` (default: vertically centred). Optional recolour."""
    sw, sh, px = _load(src)
    ys = [y for y in range(sh) for x in range(sw) if px[y][x][3] > 0]
    xs = [x for y in range(sh) for x in range(sw) if px[y][x][3] > 0]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    cw, ch = x1 - x0 + 1, y1 - y0 + 1
    s = size / max(cw, ch)
    ow, oh = max(1, round(cw * s)), max(1, round(ch * s))
    res = blank(w, h)
    left = w // 2 - ow // 2 + dx
    top = (foot - oh) if foot is not None else (h - oh) // 2
    for y in range(oh):
        for x in range(ow):
            p = px[y0 + min(ch - 1, int(y / s))][x0 + min(cw - 1, int(x / s))]
            if p[3] and 0 <= top + y < h and 0 <= left + x < w:
                if hue is not None or sat != 1.0 or val != 1.0:
                    hh, ss, vv = colorsys.rgb_to_hsv(p[0] / 255, p[1] / 255, p[2] / 255)
                    hh = hue if hue is not None else hh
                    r, g, b = (int(c * 255) for c in colorsys.hsv_to_rgb(hh, min(1, ss * sat), min(1, vv * val)))
                    p = (r, g, b, p[3])
                res[top + y][left + x] = p
    return _save(out, w, h, res)


def composite(layers: list[tuple[str, int, int]], out: str) -> str:
    """Alpha-composite images (path, dx, dy) in order onto the first one's canvas."""
    w, h, base = _load(layers[0][0])
    res = blank(w, h)
    for path, ox, oy in layers:
        lw, lh, px = _load(path)
        for y in range(lh):
            for x in range(lw):
                r, g, b, a = px[y][x]
                ty, tx = y + oy, x + ox
                if a == 0 or not (0 <= ty < h and 0 <= tx < w):
                    continue
                br, bg, bb, ba = res[ty][tx]
                al = a / 255
                res[ty][tx] = (int(r * al + br * (1 - al)), int(g * al + bg * (1 - al)), int(b * al + bb * (1 - al)),
                               max(a, ba))
    return _save(out, w, h, res)


def flip(src: str, out: str) -> str:
    """Horizontally mirrored copy."""
    w, h, px = _load(src)
    return _save(out, w, h, [list(reversed(r)) for r in px])


def facing_iso(src: str, stem: str) -> list[str]:
    """Iso avatar render tiles 0..3 (up, right, down, left). Screen-wise up/left walk towards the top-left
    and right/down towards the bottom, so tiles use the sprite and its mirror image."""
    a = _save(f"remastered/{stem}-iso.png", *_load(src))
    b = flip(src, f"remastered/{stem}-iso-flip.png")
    return [b, a, a, b]
