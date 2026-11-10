"""Python-side parity check for VectorizedGame.observe().

Renders the tiled parity fixture through the bindings with every available
backend and checks they agree with the SDL CPU reference:
  - render_backend='cuda' (host and device=True) is pixel-identical
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

    print(f"python tiled parity OK ({'sdl+cuda' if cuda is not None else 'sdl only'}, {NUM_ENVS} envs, {STEPS} steps)")


if __name__ == "__main__":
    sys.exit(main())
