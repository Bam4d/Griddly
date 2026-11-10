// Render-only throughput of the three tiled renderers on minigrid-spiders
// level 4. The envs take a random move before every frame (step time is not
// counted). Prints two tables:
//   composite fps  - frames/s, one frame = all envs tiled into one image
//   env-frames/s   - composite fps x number of envs (per-env observations/s)
// Columns:
//   sdl_cpu      SDL software renderer, every env drawing into its rect of the composite
//   sdl_gpu      SDL GPU renderer (opengles2), one shared target + one readback per frame
//   sdl_gpu_nrb  same without the readback (frame stays on the GPU)
//   cuda_host    CUDA tile-gather kernel + D2H copy of the frame
//   cuda_device  CUDA, frame stays on the GPU
// The SDL GPU columns need the GPU-enabled SDL (build with build_gpu.sh);
// they print '-' when no GPU render driver is available.
// Usage (from the repo root): bench_tiled_backends [frames] [threads]
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <vector>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Observers/TiledRenderer.hpp"
#include "Griddly/Core/Observers/TiledSDLRenderer.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"

using namespace griddly;
using clk = std::chrono::steady_clock;

static uint32_t gThreads = 16;

// The envs are stepped with random moves before every frame (not timed), so
// renderers that skip unchanged pixels are measured on a changing scene.
static VectorizedEnv* gEnv = nullptr;
static std::mt19937 gRng(7);

static void stepEnvs() {
  std::vector<std::vector<std::vector<int32_t>>> actions(gEnv->numEnvs());
  for (auto& a : actions) a.push_back({(int32_t)(1 + gRng() % 4)});
  gEnv->step(actions);
}

static double fps(TiledRenderer& r, int frames, bool device) {
  device ? r.renderDevice(true, gThreads) : r.render(true, gThreads);  // warmup + lazy init
  double seconds = 0;
  for (int i = 0; i < frames; i++) {
    stepEnvs();
    auto t0 = clk::now();
    device ? r.renderDevice(true, gThreads) : r.render(true, gThreads);
    seconds += std::chrono::duration<double>(clk::now() - t0).count();
  }
  return frames / seconds;
}

static void setEnv(const char* name, const char* value) {
  if (value != nullptr) setenv(name, value, 1);
  else unsetenv(name);
}

static void cell(double v) {
  if (v > 0) printf(" %12.1f", v);
  else printf(" %12s", "-");
}

int main(int argc, char** argv) {
  int frames = argc > 1 ? atoi(argv[1]) : 50;
  gThreads = argc > 2 ? (uint32_t)atoi(argv[2]) : 16;
  ResourceConfig rc{"resources/games", "resources/images", ""};
  auto gdy = std::make_shared<GDYFactory>(std::make_shared<ObjectGenerator>(), std::make_shared<TerminationGenerator>(), rc);
  gdy->initializeFromFile("resources/games/Single-Player/Mini-Grid/minigrid-spiders.yaml");
  std::mt19937 rng(42);

  struct Row {
    uint32_t envs;
    std::vector<double> fps;
    double frameMB;
  };
  std::vector<Row> rows;
  printf("threads=%u\ncomposite fps (one frame = all envs tiled)\n%-6s %12s %12s %12s %12s %12s %10s\n", gThreads, "envs", "sdl_cpu", "sdl_gpu", "sdl_gpu_nrb",
         "cuda_host", "cuda_device", "frame_MB");
  for (uint32_t n : {1u, 4u, 16u, 64u, 256u, 1024u}) {
    VectorizedEnv env(gdy, n, std::min(32u, n));
    env.loadLevel(4);
    env.seed(1234);
    std::vector<std::vector<std::vector<int32_t>>> actions(n);
    for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % 4)});
    env.step(actions);
    gEnv = &env;
    std::vector<std::shared_ptr<Grid>> grids;
    for (uint32_t i = 0; i < n; i++) grids.push_back(env.grid(i));
    const int slowFrames = std::max(3, n >= 256 ? frames / 4 : frames);

    TiledRendererConfig cfg;
    cfg.backend = TiledRenderBackend::SDL;
    double sdlCpu = 0, sdlGpu = 0, sdlGpuNoReadback = 0;
    {
      setEnv("GRIDDLY_SDL_FORCE_CPU", "1");
      TiledSDLRenderer sdl(gdy, n, grids, cfg);
      sdlCpu = fps(sdl, slowFrames, false);
      setEnv("GRIDDLY_SDL_FORCE_CPU", nullptr);
    }
    {
      TiledSDLRenderer sdl(gdy, n, grids, cfg);
      if (sdl.isUsingGPU()) {
        sdlGpu = fps(sdl, slowFrames, false);
        setEnv("GRIDDLY_SDL_GPU_NO_READBACK", "1");
        sdlGpuNoReadback = fps(sdl, slowFrames, false);
        setEnv("GRIDDLY_SDL_GPU_NO_READBACK", nullptr);
      }
    }
    double cudaHost = 0, cudaDevice = 0, frameMB = 0;
    if (tiledCUDAAvailable()) {
      cfg.backend = TiledRenderBackend::CUDA;
      auto cuda = createTiledRenderer(gdy, n, grids, cfg);
      cudaHost = fps(*cuda, frames, false);
      cudaDevice = fps(*cuda, frames * 4, true);
      frameMB = cuda->width() * (double)cuda->height() * 4 / 1e6;
    }
    rows.push_back({n, {sdlCpu, sdlGpu, sdlGpuNoReadback, cudaHost, cudaDevice}, frameMB});
    printf("%-6u", n);
    for (double v : rows.back().fps) cell(v);
    printf(" %10.1f\n", frameMB);
    fflush(stdout);
  }

  printf("\nenv-frames/s (composite fps x envs)\n%-6s %12s %12s %12s %12s %12s\n", "envs", "sdl_cpu", "sdl_gpu",
         "sdl_gpu_nrb", "cuda_host", "cuda_device");
  for (const auto& row : rows) {
    printf("%-6u", row.envs);
    for (double v : row.fps) cell(v * row.envs);
    printf("\n");
  }
}
