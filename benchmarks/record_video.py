import sys, time
sys.path.insert(0, '/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/Release/bin')
import numpy as np
import torch
import cv2
import python_griddly as g

ROOT = '/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources'
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/griddly_256envs.mp4'
N_ENVS = 256
FPS = 30
SECONDS = 6
LEVEL = 4

r = g.GDYReader(ROOT + '/games', ROOT + '/images', '')
gdy = r.load(ROOT + '/games/Single-Player/Mini-Grid/minigrid-spiders.yaml')
vg = gdy.create_vectorized(N_ENVS, 32, 'Sprite2D')
vg.load_level(LEVEL)
vg.seed(1234)

frames = SECONDS * FPS
# step once to size the writer after first observe
acts = np.zeros((N_ENVS, 1, 1), dtype=np.int32)
vg.step(acts)
cap = vg.observe()
img = torch.from_dlpack(cap).to(torch.uint8).numpy()  # (H, W, 4) uint8 RGBA
h, w = img.shape[:2]
print(f'frame {w}x{h}, recording {frames} frames @ {FPS}fps -> {OUT}')
import imageio
writer = imageio.get_writer(OUT, fps=FPS, codec='libx264', macro_block_size=None)
print('imageio writer ok')

rng = np.random.default_rng(0)
t0 = time.perf_counter()
for i in range(frames):
    acts[:, 0, 0] = rng.integers(1, 5, size=N_ENVS, dtype=np.int32)
    vg.step(acts)
    cap = vg.observe()
    img = torch.from_dlpack(cap).to(torch.uint8).numpy()
    bgr = img[..., :3]  # drop alpha, RGB for imageio
    writer.append_data(bgr)
    if i % 30 == 0:
        print(f'  frame {i}/{frames} ({(time.perf_counter()-t0):.1f}s)')
writer.close()
dt = time.perf_counter() - t0
print(f'done: {frames} frames in {dt:.1f}s ({frames/dt:.1f} fps end-to-end) -> {OUT}')
