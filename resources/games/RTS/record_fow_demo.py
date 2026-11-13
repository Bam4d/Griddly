"""Record a demo video of GriddlyRTS-FoW (scripted policy vs scripted policy).

Both players are driven by ``fow_policy.RTSPolicy`` and act simultaneously every tick:
harvest their corner, build a barracks, train combat units, then attack the enemy
base until one base is destroyed.

Layout per frame (centered on a dark padded canvas):
    +------------------------------------------------------+
    |   P1 PlayerView             P2 PlayerView            |
    |   P1 mini-map + caption     P2 mini-map + caption    |
    |                                                      |
    |            GlobalMap (both cameras outlined)         |
    |            tick / winner line                        |
    +------------------------------------------------------+

Every picture is rendered by the engine from the GDY's observers and pasted as-is (only scaled):
    PlayerView  each player's camera window (cam_x / cam_y globals, panned with move_camera),
                under that player's fog of war, with health bars and ownership shadows
                (egocentric: your units green, the enemy red)
    MiniMap     the whole map under that player's fog of war, with their camera outlined
    GlobalMap   the whole map without fog, both players' cameras outlined (P1 green, P2 red)

Output: GriddlyRTS-FoW-demo.mp4 next to GriddlyRTS-FoW.yaml.

    GRIDDLY_SDL_WINDOW=1 python record_fow_demo.py [level]     # level index, default 0 (Crossroads)
"""
import os
import sys

os.environ.setdefault("GRIDDLY_SDL_WINDOW", "1")

import numpy as np
import imageio.v2 as imageio
from PIL import Image, ImageDraw, ImageFont

from griddly import GriddlyLoader

HERE = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, HERE)
from fow_policy import Match  # noqa: E402

OUT = os.path.join(HERE, "GriddlyRTS-FoW-demo.mp4")
FPS = 8
FONT_SIZE = 20
PAD = 30
BG = (14, 14, 18)
TEXT = (235, 235, 220)
TILE = 16          # Sprite2D tile size
SCALE = 2          # upscale factor for the camera views (PlayerView is 24x15 tiles of 16px)
MINI_SCALE = 2     # upscale factor for the per-player mini-maps
MAX_TICKS = 900
HOLD_FRAMES = 3 * FPS


def load_font(size):
    for path in (
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ):
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


FONT = load_font(FONT_SIZE)


def to_img(obs):
    a = np.asarray(obs.to_image() if hasattr(obs, "to_image") else obs)
    if a.ndim == 3 and a.shape[2] == 4:
        a = a[..., :3]
    return np.ascontiguousarray(a)


def upscale(im, scale):
    return np.repeat(np.repeat(im, scale, axis=0), scale, axis=1)


def wrap(lines, width):
    """Word-wrap caption lines so each fits in `width` pixels."""
    out = []
    for line in lines:
        cur = ""
        for word in line.split(" "):
            cand = f"{cur} {word}" if cur else word
            if FONT.getlength(cand) <= width - 20 or not cur:
                cur = cand
            else:
                out.append(cur)
                cur = word
        out.append(cur)
    return out


def text_block(width, lines, n_lines, fill):
    line_h = FONT_SIZE + 8
    block = Image.new("RGB", (width, n_lines * line_h + 12), BG)
    draw = ImageDraw.Draw(block)
    for i, line in enumerate(wrap(lines, width)[:n_lines]):
        draw.text((10, 6 + i * line_h), line, font=FONT, fill=fill)
    return np.asarray(block)


CAPTION_LINES = 4


def compose(p1_view, p2_view, p1_mini, p2_mini, g_img, cap1, cap2, status):
    p1s, p2s = upscale(p1_view, SCALE), upscale(p2_view, SCALE)
    m1, m2 = upscale(p1_mini, MINI_SCALE), upscale(p2_mini, MINI_SCALE)
    pw = p1s.shape[1]
    # the global map is scaled to roughly the width of one player view
    gs = upscale(g_img, max(1, round(pw / g_img.shape[1])))
    gap = 60
    top_w = 2 * pw + gap
    # under each view: the player's mini-map on the left, the caption beside it
    cap_w = pw - m1.shape[1] - 10
    c1 = text_block(cap_w, cap1, CAPTION_LINES, TEXT)
    c2 = text_block(cap_w, cap2, CAPTION_LINES, TEXT)
    st = text_block(max(gs.shape[1], top_w), [status], 1, (220, 220, 220))
    under_h = max(m1.shape[0], c1.shape[0])
    row_h = p1s.shape[0] + 10 + under_h
    width = max(top_w, gs.shape[1]) + 2 * PAD
    height = PAD + row_h + PAD + gs.shape[0] + st.shape[0] + PAD
    height += height % 2
    canvas = np.zeros((height, width, 3), dtype=np.uint8)
    canvas[:] = BG
    # player 1 on the left, player 2 on the right
    x0 = (width - top_w) // 2
    for x, view, mini, cap in ((x0, p1s, m1, c1), (x0 + pw + gap, p2s, m2, c2)):
        canvas[PAD:PAD + view.shape[0], x:x + pw] = view
        y = PAD + view.shape[0] + 10
        canvas[y:y + mini.shape[0], x:x + mini.shape[1]] = mini
        canvas[y:y + cap.shape[0], x + mini.shape[1] + 10:x + mini.shape[1] + 10 + cap.shape[1]] = cap
    # global map below, centered, with the status line under it
    y = PAD + row_h + PAD
    gx = (width - gs.shape[1]) // 2
    canvas[y:y + gs.shape[0], gx:gx + gs.shape[1]] = gs
    sx = (width - st.shape[1]) // 2
    canvas[y + gs.shape[0]:y + gs.shape[0] + st.shape[0], sx:sx + st.shape[1]] = st
    return canvas


def level_names(path):
    """Map names from the `# Name | blurb` comments above each level in the GDY."""
    names = []
    with open(path) as f:
        for line in f:
            if line.startswith("    # ") and "|" in line:
                names.append(line[6:].split("|")[0].strip())
    return names


def main():
    level = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    path = os.path.join(HERE, "GriddlyRTS-FoW.yaml")
    name = (level_names(path) + [f"level {level}"] * (level + 1))[level]
    gdy = GriddlyLoader().load(path)
    game = gdy.create_game("GlobalMap")                  # global observer: whole map, both cameras
    p1 = game.register_player("Player 1", "PlayerView")  # camera window + fog of war, per player
    p2 = game.register_player("Player 2", "PlayerView")
    game.init(False)
    game.load_level(level)
    game.reset()

    match = Match(game, p1, p2, seed=0)
    frames = []

    def record(cap1, cap2, status):
        frames.append(compose(to_img(p1.observe()), to_img(p2.observe()),
                              to_img(game.observe_player(1, "MiniMap")), to_img(game.observe_player(2, "MiniMap")),
                              to_img(game.observe()), cap1, cap2, status))

    world = match.world(1)
    start = ["START", f"res={world.resources(1)}", "camera on own base"]
    record(["P1 " + start[0]] + start[1:], ["P2 " + start[0]] + start[1:],
           f"{name}  tick 0   map: P1 green, P2 red   views: self green, enemy red, fog of war")

    info = {}
    for _ in range(MAX_TICKS):
        captions, _, info = match.step()
        if match.terminated:
            break
        tick = match.world(1).tick
        record(["P1 " + captions[1][0]] + captions[1][1:],
               ["P2 " + captions[2][0]] + captions[2][1:],
               f"{name}  tick {tick}   map: P1 green, P2 red   views: self green, enemy red, fog of war")

    if match.terminated:
        # the game process is now terminal; render the last state before it is reset
        results = info.get("PlayerResults", {})
        winner = [pid for pid, r in results.items() if r == "Win"]
        end = f"GAME OVER - P{winner[0]} destroyed the enemy base" if winner else "GAME OVER"
        cap = {pid: [f"P{pid} {results.get(str(pid), '?').upper()}"] for pid in (1, 2)}
    else:
        end = f"stopped after {MAX_TICKS} ticks"
        cap = {1: ["P1 (time limit)"], 2: ["P2 (time limit)"]}
    for _ in range(HOLD_FRAMES):
        record(cap[1], cap[2], end)

    imageio.mimsave(OUT, frames, fps=FPS, macro_block_size=1)
    print(f"wrote {OUT}: {len(frames)} frames, {frames[0].shape[1]}x{frames[0].shape[0]}, {end}")


if __name__ == "__main__":
    main()
