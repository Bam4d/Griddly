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
env -u SDL_VIDEODRIVER GRIDDLY_SDL_GPU_NO_READBACK=1 ./benchmarks/bench_render_only 60  # GPU, no sync
env -u SDL_VIDEODRIVER GRIDDLY_SDL_BATCH=1 ./benchmarks/bench_render_only 60       # GPU batch (shared target)

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
| `GRIDDLY_SDL_FORCE_CPU=1` | skip GPU, CPU software renderer + memcpy composite |
| `GRIDDLY_SDL_DRIVER=<name>` | force one SDL render driver (e.g. `opengles2`) |
| `GRIDDLY_SDL_GPU_NO_READBACK=1` | no `glReadPixels` (draw calls only queue async; fps is not "real") |
| `GRIDDLY_SDL_BATCH=1` | all envs draw into the one composite target via viewport |
| `SDL_VIDEODRIVER=offscreen` | required headless; registers opengles2/vulkan/gpu drivers |
| `EGL_PLATFORM=surfaceless` | EGL surfaceless context on the H100 |
| `BENCH_OUT=<path>` | where `bench_tiled` writes its tables |

## 4. Reading the results

- **fps numbers are composite fps**: one frame = all N envs tiled into one
  image. env-frames/s = fps x N. Raw Mpix/s = fps x frame_pixels / 1e6.
- The `cksum` printed by `bench_render_only` is a pixel checksum — if it's
  tiny/zero the frame drew nothing (guard against "fast because empty").
- Rendering does NOT scale with envs: frame size grows with N, so composite
  fps falls ~linearly with N. Use many envs for `step` throughput, not render.
- GPU + readback is sync-bound (glReadPixels of the whole frame each frame);
  CPU software often beats it at N>=4. GPU no-readback is async queueing only.

## 5. Data files

`benchmarks/h100_*.txt` — saved sweeps (cpu / gpu_egl / gpu_readback_on/off /
gpu_batch / fixed_* on 2026-11-06 and 2026-11-10).
`benchmarks/h100_render_composite_fps_2026-11-10.png` — chart
(regenerate: `python3 benchmarks/make_render_chart.py`).
`benchmarks/griddly_{16,256}envs_*.mp4` — tiled videos.
