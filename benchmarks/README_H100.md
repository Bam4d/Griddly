# Running the Griddly benchmarks on an H100 node

## 0. Get an H100 node

```bash
srun -p h100 -A robotics --qos=dev --gres=gpu:h100:1 --time=02:00:00 \
  --container-image=/mnt/vast/containers/robotics/robotics-simulator_v0.0.9-amd64.sqsh \
  --container-mounts=/mnt/vast/home/bam4d:/mnt/vast/home/bam4d \
  --container-workdir=/mnt/vast/home/bam4d \
  --pty bash
```

(Or just work on a node that already has an H100, like the current session.)

Check the GPU and EGL vendor libs:

```bash
nvidia-smi | head -3
ls /usr/share/glvnd/egl_vendor.d/   # should list 10_nvidia.json
```

## 1. Build

From the repo root (`vibe_main` worktree):

```bash
# main library + python module (conan SDL here is CPU-only - fine for step sims)
cd build && ninja Griddly_static python_griddly && cd ..

# benchmarks: two link modes
bash benchmarks/build.sh bench_vectorized     # links conan (CPU-only) SDL -> CPU renderer
bash benchmarks/build.sh bench_tiled
bash benchmarks/build.sh bench_render_only

bash benchmarks/build_gpu.sh bench_render_only # links GPU-enabled SDL -> EGL renderer
bash benchmarks/build_gpu.sh bench_tiled_backends  # SDL CPU, SDL GPU and CUDA side by side
```

`build_gpu.sh` needs a GPU-enabled SDL3 static build. If `~/sdl-gpu/install` is
gone (e.g. fresh node), rebuild it once:

```bash
SDL_SRC=$HOME/.conan2/p/sdl4d8260cdd8b13/s/src   # SDL 3.4.16 source from conan cache
cmake -S $SDL_SRC -B ~/sdl-gpu/build \
  -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF -DSDL_STATIC=ON \
  -DSDL_OPENGL=ON -DSDL_OPENGLES=ON -DSDL_VULKAN=ON \
  -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_DBUS=OFF -DSDL_ALSA=OFF \
  -DSDL_PULSEAUDIO=OFF -DSDL_LIBUDEV=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
  -DSDL_UNIX_CONSOLE_BUILD=ON -DCMAKE_INSTALL_PREFIX=~/sdl-gpu/install
cmake --build ~/sdl-gpu/build -j$(nproc) && cmake --install ~/sdl-gpu/build
```

The conan SDL (`~/.conan2/p/b/sdl8c0255475fc2e`) is built with
`opengl=False opengles=False vulkan=False`, so GPU render drivers are compiled
out — that's why we need the second SDL build for EGL rendering.

## 2. Run

All benchmarks use **spiders level 4** (19x18 tiles @24px, the biggest level).

Headless GPU rendering requires the `offscreen` SDL video driver; our code sets
the hint automatically when `DISPLAY` is unset, but you can also export it:

```bash
export SDL_VIDEODRIVER=offscreen
export EGL_PLATFORM=surfaceless
```

```bash
# --- simulation only (no rendering; works with the conan CPU SDL) ---
./benchmarks/bench_vectorized Single-Player/Mini-Grid/minigrid-spiders.yaml 4 300

# --- render-only sweep (envs x threads), prints composite fps + pixel checksum ---
env -u SDL_VIDEODRIVER GRIDDLY_SDL_FORCE_CPU=1 ./benchmarks/bench_render_only 60   # CPU path
env -u SDL_VIDEODRIVER ./benchmarks/bench_render_only 60                           # GPU EGL + readback
env -u SDL_VIDEODRIVER GRIDDLY_SDL_GPU_NO_READBACK=1 ./benchmarks/bench_render_only 60  # GPU, no readback
env -u SDL_VIDEODRIVER ./benchmarks/bench_tiled_backends 40 16                  # all three renderers, run from repo root

# --- step + render + combined ---
BENCH_OUT=/tmp/tiled.txt ./benchmarks/bench_tiled Single-Player/Mini-Grid/minigrid-spiders.yaml 4 200 Sprite2D

# --- python bindings (nanobind, VectorizedGame, DLPack observe) ---
python3.12 benchmarks/bench_py_vectorized.py Single-Player/Mini-Grid/minigrid-spiders.yaml 4 300

# --- record a tiled mp4 of N envs (edit N_ENVS/SECONDS/LEVEL in the file) ---
python3.12 benchmarks/record_video.py benchmarks/out.mp4
```

> Use the GPU-linked binary (`build_gpu.sh`) for any GPU render run; the
> conan-SDL binary can only do the CPU software renderer.

## 3. Env vars that change renderer behaviour

| var | effect |
|---|---|
| `GRIDDLY_TILED_BACKEND=sdl\|cuda\|auto` | tiled renderer backend (default `auto`: CUDA when built with CUDA, a device is visible and the observer is supported) |
| `GRIDDLY_SDL_FORCE_CPU=1` | skip GPU, CPU software renderer (each env draws into its rect of the composite) |
| `GRIDDLY_SDL_DRIVER=<name>` | force one SDL render driver (e.g. `opengles2`) |
| `GRIDDLY_SDL_GPU_NO_READBACK=1` | GPU: skip the `glReadPixels`; the frame is still flushed and finished on the GPU |
| `SDL_VIDEODRIVER=offscreen` | required headless; registers opengles2/vulkan/gpu drivers |
| `EGL_PLATFORM=surfaceless` | EGL surfaceless context on the H100 |
| `BENCH_OUT=<path>` | where `bench_tiled` writes its tables |

## 3b. Tiled renderers: SDL CPU, SDL GPU and CUDA

`createTiledRenderer()` (`src/Griddly/Core/Observers/TiledRenderer.hpp`) picks a backend:

- **SDL** (`TiledSDLRenderer`): one SDL observer per env. Has every observer
  feature and is where 3D and effects work goes. It runs on the GPU when SDL
  has a GPU render driver, otherwise on the CPU:
  - **GPU**: one renderer and one composite target. All env observers share
    the renderer and its sprite textures, and each draws its tile through a
    viewport. Render items are built on a thread pool; draw calls come from the
    calling thread, since a GL context belongs to one thread. Then one
    `glReadPixels` reads the whole frame into the host buffer.
  - **CPU**: each env's software renderer draws straight into its rect of the
    composite on a persistent thread pool, so there is no composite copy.
- **CUDA** (`CUDA/TiledCUDARenderer`): one tile-gather kernel renders every env.
  The CPU only writes per-cell sprite codes. It supports Sprite2D observers
  without avatar tracking, offsets, sprite scale, player highlights or
  isometric views; anything else falls back to SDL under `auto`.
  - Built when CMake finds nvcc. Turn it off with `-DGRIDDLY_ENABLE_CUDA=OFF`.

Output parity is checked by `TiledRendererTest` (C++) and
`tests/python_tiled_parity.py` (bindings). Both use the fixture
`tests/resources/tiled_parity.yaml`, which covers WALL_2/WALL_16, all four
rotations, a 5-object stack, render tile ids and moving objects. The checks:

- SDL CPU, the reference, is pinned by the golden image
  `tests/resources/observer/tiled/tiled_parity_sprite2d_level0.png`, and must
  equal each env rendered by a standalone observer.
- CUDA is pixel-identical to SDL CPU across envs × steps, on the fixture and
  4 stock games. `renderDevice()` matches `render()`.
- SDL GPU is within 3 LSB per channel of SDL CPU. GL blends in floating point,
  so the error is about 1 LSB per translucent layer.

Missing backends skip. Set `GRIDDLY_REQUIRE_TILED_BACKENDS=cuda,sdl_gpu` to
make them fail instead, as the GPU CI job does. If an intended rendering change
breaks the golden image, regenerate it with `GRIDDLY_UPDATE_GOLDEN_IMAGES=1`.

Python usage: `gdy.create_vectorized(n, threads, 'Sprite2D', render_backend='auto'|'sdl'|'cuda')`.
`vg.observe(device=True)` returns a CUDA uint8 tensor that never leaves the GPU.

`bench_tiled_backends` measures render-only throughput on spiders level 4
(H100, 16 threads = 8 physical cores of a Xeon 8462Y+, 2026-11-10;
`benchmarks/h100_tiled_backends_2026-11-10.txt`). Every env takes a random
move before each frame; the step itself is not timed. The columns are:

- `sdl_gpu_nrb`: no readback, but the frame is still flushed and `glFinish`ed.
- `cuda_device`: the frame stays on the GPU.
- `cuda_host`: includes the D2H copy.

**env-frames/s** is composite fps × envs: per-env observations rendered per
second, the number to compare against simulation throughput.

| envs | sdl_cpu | sdl_gpu | sdl_gpu_nrb | cuda_host | cuda_device |
|---|---|---|---|---|---|
| 1 | 40,416 | 7,983 | 10,428 | 26,707 | 64,299 |
| 4 | 68,131 | 8,853 | 14,632 | 41,592 | 133,030 |
| 16 | 129,334 | 10,639 | 25,842 | 45,584 | 192,641 |
| 64 | 195,112 | 13,608 | 31,310 | 54,746 | 431,205 |
| 256 | 222,188 | 14,460 | 34,953 | 59,156 | 592,101 |
| 1024 | 183,970 | 12,949 | 31,726 | 60,286 | 616,687 |

**Composite fps** counts one frame as all envs tiled into one image (frame MB
is that image's size):

| envs | sdl_cpu | sdl_gpu | sdl_gpu_nrb | cuda_host | cuda_device | frame MB |
|---|---|---|---|---|---|---|
| 1 | 40,416 | 7,983 | 10,428 | 26,707 | 64,299 | 0.8 |
| 4 | 17,033 | 2,213 | 3,658 | 10,398 | 33,258 | 3.2 |
| 16 | 8,083 | 665 | 1,615 | 2,849 | 12,040 | 12.7 |
| 64 | 3,049 | 213 | 489 | 855 | 6,738 | 50.9 |
| 256 | 868 | 56.5 | 137 | 231 | 2,313 | 203.7 |
| 1024 | 180 | 12.6 | 31.0 | 58.9 | 602 | 814.4 |

What the SDL renderer does per frame:

- **Incremental repaint (CPU).** Each env draws into its own rect of the
  composite, so the previous frame's pixels are still there. Items are
  grouped by grid cell, and only cells whose sprites changed are restored
  from the base layer and redrawn; typically a handful of the 342 cells per
  env. This is what took SDL CPU from about 35k to about 200k env-frames/s.
  It applies when every sprite sits inside one cell (Sprite2D and Block2D
  without avatar rotation); other scenes, or a reset, redraw the whole env.
  `GRIDDLY_SDL_NO_DIRTY_CELLS=1` turns it off.
- **Base layer.** The clear colour and background tile are the same in every
  frame, so they are rendered once. The CPU path copies them in (per dirty
  cell); the GPU path draws them as one quad per env.
- **GPU batching.** For cell-aligned scenes, all envs are drawn as one
  `SDL_RenderGeometry` call per (z, texture) run instead of one draw call per
  sprite. Other scenes fall back to per-env drawing.
- **One readback (GPU).** `glReadPixels` reads the whole frame straight into
  the host buffer, at about 21 GB/s.
- **No per-frame allocations** on the CPU path, about two on the GPU path
  (SDL's command queue).

How to read it:

- **SDL CPU without the incremental repaint** was limited by
  software-rasterising every sprite: about 1.1 µs per sprite × 80 sprites
  per env. It levelled off at about 35k env-frames/s on the 8 physical cores
  this job gets (hyperthreads add little). Writing the whole frame each time
  would itself cap out at 60-120k env-frames/s from memory bandwidth (an 8-core
  memcpy / streaming-store test). Repainting only changed cells avoids both
  limits.
- **SDL CPU** drops at 1024 envs (814 MB frame) because the composite no
  longer fits in cache, so even small writes miss.
- **SDL GPU** is limited by the frame readback, about 2/3 of the frame time
  from 64 envs up. A PBO readback was tried and was slower on this driver.
  Without a readback it reaches about 30k env-frames/s. It is the place for
  draw-heavy work (3D, effects, shaders), not for plain 2D sprites.
- **CUDA** is fastest from 4 envs up. With the frame left on the GPU it
  reaches 600k env-frames/s, about 3x the best SDL CPU result, and the
  frame is already where a GPU model needs it. `cuda_host` is limited by PCIe;
  for training keep frames on the GPU with `observe(device=True)`.

Why some older numbers showed ~1M fps: the 2026-11-10 `*gpu_readback_off*`
and `*norb*` sweeps ran with `GRIDDLY_SDL_GPU_NO_READBACK=1`. That only
queued SDL commands and never flushed or executed them, so it timed command
recording, not rendering. Those frames were also empty (see below). The
no-readback mode now flushes and waits for the GPU.

| var | effect |
|---|---|
| `GRIDDLY_SDL_NO_BASE_LAYER=1` | draw clear + background every frame (debug / A-B) |
| `GRIDDLY_SDL_NO_DIRTY_CELLS=1` | CPU: redraw every cell every frame instead of only changed ones (debug / A-B) |
| `GRIDDLY_SDL_NO_BATCH=1` | GPU: draw each env with its own draw calls (debug / A-B) |

The `gpu_*` sweeps in section 5 predate these fixes. That SDL GPU path drew
empty env views, because each observer had its own renderer and SDL textures
belong to one renderer. Use them for timing history only.

## 4. Reading the results

- **fps numbers are composite fps**: one frame = all N envs tiled into one
  image. env-frames/s = fps x N (`bench_tiled_backends` prints both). Raw Mpix/s = fps x frame_pixels / 1e6.
- The `cksum` printed by `bench_render_only` is a pixel checksum — if it's
  tiny/zero the frame drew nothing (guard against "fast because empty").
- Rendering does NOT scale with envs: frame size grows with N, so composite
  fps falls ~linearly with N. Use many envs for `step` throughput, not render.
- SDL GPU + readback is sync-bound (glReadPixels of the whole frame each frame);
  the threaded CPU software renderer beats it at N>=4. See section 3b.

## 5. Data files

`benchmarks/h100_tiled_backends_2026-11-10.txt` — SDL vs CUDA backend sweep (section 3b).
`benchmarks/h100_*.txt` — saved sweeps (cpu / gpu_egl / gpu_readback_on/off /
gpu_batch / fixed_* on 2026-11-06 and 2026-11-10).
`benchmarks/h100_render_composite_fps_2026-11-10.png` — chart
(regenerate: `python3 benchmarks/make_render_chart.py`).
`benchmarks/griddly_{16,256}envs_*.mp4` — tiled videos.
