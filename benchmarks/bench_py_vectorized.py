#!/usr/bin/env python3.12
"""Benchmark the Python VectorizedGame binding: step FPS and observe() FPS,
plus the DLPack->torch path, on the biggest spiders level."""
import sys, time
sys.path.insert(0, '/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/Release/bin')
import numpy as np
import torch
import python_griddly as g

ROOT = '/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources'

def main():
    game = sys.argv[1] if len(sys.argv) > 1 else 'Single-Player/Mini-Grid/minigrid-spiders.yaml'
    level = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    steps = int(sys.argv[3]) if len(sys.argv) > 3 else 500

    r = g.GDYReader(ROOT + '/games', ROOT + '/images', '')
    gdy = r.load(ROOT + '/games/' + game)
    n_actions = 4  # move: ids 1..4

    print(f'game={game} level={level} steps={steps}')
    print(f'{"envs":>6} {"thr":>4} {"env-steps/s":>12} {"us/step":>9}')
    for n, t in [(1, 1), (4, 4), (8, 8), (16, 8), (32, 16), (64, 16), (128, 32), (256, 32)]:
        vg = gdy.create_vectorized(n, t, 'Sprite2D')
        vg.load_level(level)
        vg.seed(1234)
        acts = np.zeros((n, 1, 1), dtype=np.int32)
        for i in range(20):
            acts[:, 0, 0] = 1 + (i % n_actions)
            vg.step(acts)
        t0 = time.perf_counter()
        for i in range(steps):
            acts[:, 0, 0] = 1 + (i % n_actions)
            vg.step(acts)
        dt = time.perf_counter() - t0
        print(f'{n:>6} {t:>4} {steps*n/dt:>12.0f} {dt*1e6/(steps*n):>9.2f}')

    # observe() benchmark + DLPack->torch
    print('\nobserve() (tiled render + torch.from_dlpack):')
    for n, t in [(1, 1), (8, 8), (16, 8), (64, 16), (256, 32)]:
        vg = gdy.create_vectorized(n, t, 'Sprite2D')
        vg.load_level(level)
        vg.seed(1234)
        vg.step(np.full((n, 1, 1), 2, dtype=np.int32))
        cap = vg.observe()
        frames = min(100, steps)
        t0 = time.perf_counter()
        for _ in range(frames):
            cap = vg.observe()
        dt = time.perf_counter() - t0
        t0 = time.perf_counter()
        for _ in range(frames):
            ten = torch.from_dlpack(vg.observe())
        dt2 = time.perf_counter() - t0
        print(f'  envs={n:>3} thr={t:>2}  render {frames/dt:>8.1f} frames/s   dlpack->torch {frames/dt2:>8.1f}/s   tensor {tuple(ten.shape)} {ten.dtype} {ten.device}')

if __name__ == '__main__':
    main()
