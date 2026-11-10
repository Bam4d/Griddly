"""Python-side parity check for VectorizedGame.observe().

Renders the tiled parity fixture through the bindings with every available
backend and checks they agree with the SDL CPU reference:
  - render_backend='cuda' (host and device=True) is pixel-identical
  - the SDL GPU renderer's device=True frame (GL shared with CUDA) equals its
    host frame
  - the DLPack tensors are uint8 with the expected shape and device
Needs only numpy; the device=True check also needs torch.
Run from the repo root: PYTHONPATH=Release/bin python tests/python_tiled_parity.py
Set GRIDDLY_REQUIRE_TILED_BACKENDS=cuda to fail instead of skipping without CUDA.
"""
import os
import sys

import numpy as np

import python_griddly as gd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIXTURE = os.path.join(ROOT, "tests", "resources", "tiled_parity.yaml")
NUM_ENVS = 6
STEPS = 8


try:
    import torch
except ImportError:
    torch = None


def to_numpy(capsule):
    """Host DLPack capsule -> numpy."""
    if torch is not None:
        return torch.from_dlpack(capsule).numpy()
    return np.from_dlpack(_CapsuleHolder(capsule))


class _CapsuleHolder:
    """numpy.from_dlpack wants an object with __dlpack__."""

    def __init__(self, capsule):
        self._capsule = capsule

    def __dlpack__(self, stream=None):
        return self._capsule

    def __dlpack_device__(self):
        return (1, 0)  # kDLCPU


def make_game(gdy, backend):
    game = gdy.create_vectorized(NUM_ENVS, 2, "Sprite2D", render_backend=backend)
    game.load_level(1)
    game.seed(1234)
    return game


def check_sdl_gpu_device(gdy, required):
    """SDL GPU renderer: observe(device=True) shares the GL frame with CUDA and
    must equal observe() exactly. Runs in a child process so the SDL CPU
    reference above (GRIDDLY_SDL_FORCE_CPU) does not apply."""
    import subprocess

    code = (
        "import os, sys, numpy as np, torch, python_griddly as gd\n"
        "os.environ.pop('GRIDDLY_SDL_FORCE_CPU', None)\n"
        f"r = gd.GDYReader({os.path.join(ROOT, 'resources', 'games')!r}, {os.path.join(ROOT, 'resources', 'images')!r}, '')\n"
        f"g = r.load({FIXTURE!r}).create_vectorized({NUM_ENVS}, 2, 'Sprite2D', render_backend='sdl')\n"
        "g.load_level(1); g.seed(1234)\n"
        "rng = np.random.default_rng(0)\n"
        "for step in range(4):\n"
        "    if step: g.step(rng.integers(1, 4, size=(g.num_envs(), 1, 1), dtype=np.int32))\n"
        "    try:\n"
        "        dev = torch.from_dlpack(g.observe(device=True))\n"
        "    except Exception as e:\n"
        "        print('SKIP', e); sys.exit(3)\n"
        "    assert dev.is_cuda and dev.dtype == torch.uint8, (dev.device, dev.dtype)\n"
        "    host = torch.from_dlpack(g.observe()).numpy()\n"
        "    assert np.array_equal(dev.cpu().numpy(), host), f'step {step}: sdl gpu device frame != host frame'\n"
        "print('OK')\n"
    )
    if torch is None:
        print("SKIP sdl gpu device=True: needs torch")
        return
    env = dict(os.environ)
    env.pop("GRIDDLY_SDL_FORCE_CPU", None)
    env.pop("SDL_VIDEODRIVER", None)
    proc = subprocess.run([sys.executable, "-c", code], env=env, capture_output=True, text=True)
    lines = [l for l in proc.stdout.splitlines() if l.strip()]
    if proc.returncode == 3:
        if "sdl_gpu_cuda" in required:
            raise AssertionError(f"sdl_gpu_cuda required but unavailable: {lines[-1] if lines else proc.stderr[-500:]}")
        print(f"SKIP sdl gpu device=True: {lines[-1] if lines else ''}")
        return
    assert proc.returncode == 0, f"sdl gpu device=True check failed:\n{proc.stdout[-2000:]}\n{proc.stderr[-2000:]}"


def main():
    os.environ.setdefault("GRIDDLY_SDL_FORCE_CPU", "1")  # SDL CPU is the reference
    os.environ.pop("GRIDDLY_TILED_BACKEND", None)  # would override render_backend below
    required = os.environ.get("GRIDDLY_REQUIRE_TILED_BACKENDS", "").split(",")
    reader = gd.GDYReader(os.path.join(ROOT, "resources", "games"), os.path.join(ROOT, "resources", "images"), "")
    gdy = reader.load(FIXTURE)

    reference = make_game(gdy, "sdl")
    try:
        cuda = make_game(gdy, "cuda")
        cuda.observe()
    except Exception as e:  # built without CUDA or no device
        if "cuda" in required:
            raise
        print(f"SKIP cuda: {e}")
        cuda = None

    rng = np.random.default_rng(0)
    for step in range(STEPS + 1):
        if step > 0:
            actions = rng.integers(1, 4, size=(NUM_ENVS, 1, 1), dtype=np.int32)
            reference.step(actions)
            if cuda is not None:
                cuda.step(actions)

        ref = to_numpy(reference.observe()).copy()
        assert reference.render_backend() == "sdl", reference.render_backend()
        assert ref.dtype == np.uint8 and ref.ndim == 3 and ref.shape[2] == 4, (ref.dtype, ref.shape)
        assert ref.shape == (reference.frame_height(), reference.frame_width(), 4)

        if cuda is not None:
            assert cuda.render_backend() == "cuda", cuda.render_backend()
            host = to_numpy(cuda.observe()).copy()
            diff = np.argwhere((host != ref).any(axis=-1))
            assert len(diff) == 0, f"step {step}: cuda host differs from sdl in {len(diff)} pixels, first {diff[0].tolist()}"

            if torch is None:
                continue  # device tensors need torch to inspect
            device_tensor = torch.from_dlpack(cuda.observe(device=True))
            assert device_tensor.is_cuda and device_tensor.dtype == torch.uint8, (device_tensor.device, device_tensor.dtype)
            assert np.array_equal(device_tensor.cpu().numpy(), ref), f"step {step}: cuda device frame differs from sdl"

    check_sdl_gpu_device(gdy, required)
    print(f"python tiled parity OK ({'sdl+cuda' if cuda is not None else 'sdl only'}, {NUM_ENVS} envs, {STEPS} steps)")


if __name__ == "__main__":
    sys.exit(main())
