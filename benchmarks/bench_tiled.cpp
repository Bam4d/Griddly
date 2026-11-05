// Benchmark: VectorizedEnv step + TiledSDLRenderer render, envs x threads.
// Measures step-only FPS, render-only FPS, and combined step+render FPS.
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"
#include "Griddly/Core/Observers/TiledSDLRenderer.hpp"

using namespace griddly;

static std::shared_ptr<GDYFactory> loadGdy(const std::string& root, const std::string& game) {
  ResourceConfig rc{root + "/games", root + "/images", ""};
  auto og = std::make_shared<ObjectGenerator>();
  auto tg = std::make_shared<TerminationGenerator>();
  auto f = std::make_shared<GDYFactory>(og, tg, rc);
  f->initializeFromFile(root + "/games/" + game);
  return f;
}

int main(int argc, char** argv) {
  std::string root = "/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources";
  std::string game = argc > 1 ? argv[1] : "Single-Player/Mini-Grid/minigrid-spiders.yaml";
  uint32_t level = argc > 2 ? std::stoul(argv[2]) : 4;
  int steps = argc > 3 ? std::atoi(argv[3]) : 500;
  std::string observerName = argc > 4 ? argv[4] : "Sprite2D";

  auto gdy = loadGdy(root, game);
  gdy->setMaxSteps(1000000);
  auto actionNames = gdy->getExternalActionNames();
  uint32_t nActions = 1;
  for (auto& name : actionNames) nActions = std::max(nActions, (uint32_t)gdy->findActionInputsDefinition(name).inputMappings.size());

  std::mt19937 rng(42);
  std::vector<uint32_t> envCounts = {1u, 4u, 8u, 16u, 32u, 64u};
  std::vector<uint32_t> threadCounts = {1u, 4u, 8u, 16u, 32u};

  FILE* out = fopen("/tmp/bench_tiled_out.txt", "w");
  fprintf(out, "game=%s level=%u steps=%d observer=%s\n", game.c_str(), level, steps, observerName.c_str());

  // helper: build env + renderer for n envs
  auto makeEnv = [&](uint32_t n, uint32_t t) {
    auto* env = new VectorizedEnv(gdy, n, t);
    env->loadLevel(level);
    env->seed(1234);
    return env;
  };

  fprintf(out, "\n=== step-only (env-steps/s) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      auto* env = makeEnv(n, nt);
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});
      for (int i = 0; i < 10; i++) env->step(actions);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < steps; i++) {
        for (auto& a : actions) a[0][0] = 1 + (int32_t)(rng() % nActions);
        env->step(actions);
      }
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.0f", (double)steps * n / secs);
      fflush(out);
      delete env;
    }
    fprintf(out, "\n");
  }

  // render benchmarks: step once to get grids, then time render only
  fprintf(out, "\n=== render-only (frames/s, all envs composited) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      auto* env = makeEnv(n, nt);
      std::vector<std::shared_ptr<Grid>> grids;
      for (uint32_t i = 0; i < n; i++) grids.push_back(env->grid(i));
      TiledSDLRendererConfig cfg;
      cfg.observerName = observerName;
      TiledSDLRenderer renderer(gdy, n, grids, cfg);
      // one warmup render
      renderer.render(true, nt);
      auto t0 = std::chrono::steady_clock::now();
      int frames = steps > 100 ? 100 : steps;
      for (int i = 0; i < frames; i++) renderer.render(true, nt);
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.1f", frames / secs);
      fflush(out);
      delete env;
    }
    fprintf(out, "\n");
  }

  // combined step+render
  fprintf(out, "\n=== step+render combined (env-steps/s, render every step) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      auto* env = makeEnv(n, nt);
      std::vector<std::shared_ptr<Grid>> grids;
      for (uint32_t i = 0; i < n; i++) grids.push_back(env->grid(i));
      TiledSDLRendererConfig cfg;
      cfg.observerName = observerName;
      TiledSDLRenderer renderer(gdy, n, grids, cfg);
      renderer.render(true, nt);
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});
      int frames = steps > 200 ? 200 : steps;
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < frames; i++) {
        for (auto& a : actions) a[0][0] = 1 + (int32_t)(rng() % nActions);
        env->step(actions);
        renderer.render(true, nt);
      }
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.0f", (double)frames * n / secs);
      fflush(out);
      delete env;
    }
    fprintf(out, "\n");
  }

  fclose(out);
  printf("done -> /tmp/bench_tiled_out.txt\n");
  return 0;
}
