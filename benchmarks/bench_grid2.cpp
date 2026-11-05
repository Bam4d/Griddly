// FPS + reset benchmark: VectorizedEnv on the biggest spiders level (4).
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"

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
  int steps = argc > 3 ? std::atoi(argv[3]) : 2000;
  int resets = argc > 4 ? std::atoi(argv[4]) : 200;

  auto gdy = loadGdy(root, game);
  gdy->setMaxSteps(1000000);
  auto actionNames = gdy->getExternalActionNames();
  uint32_t nActions = 1;
  for (auto& name : actionNames) nActions = std::max(nActions, (uint32_t)gdy->findActionInputsDefinition(name).inputMappings.size());

  std::mt19937 rng(42);
  std::vector<uint32_t> envCounts = {1u, 4u, 8u, 16u, 32u, 64u, 128u, 256u};
  std::vector<uint32_t> threadCounts = {1u, 4u, 8u, 16u, 32u};

  FILE* out = fopen("/tmp/bench_grid_out.txt", "w");
  fprintf(out, "game=%s level=%u steps=%d resets=%d\n", game.c_str(), level, steps, resets);

  fprintf(out, "\n=== step throughput (env-steps/s) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      VectorizedEnv env(gdy, n, nt);
      env.loadLevel(level);
      env.seed(1234);
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});
      for (int i = 0; i < 20; i++) env.step(actions);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < steps; i++) {
        for (auto& a : actions) a[0][0] = 1 + (int32_t)(rng() % nActions);
        env.step(actions);
      }
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.0f", (double)steps * n / secs);
      fflush(out);
    }
    fprintf(out, "\n");
  }

  fprintf(out, "\n=== reset throughput (env-resets/s, reset() over all envs) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      VectorizedEnv env(gdy, n, nt);
      env.loadLevel(level);
      env.seed(1234);
      for (int i = 0; i < 5; i++) env.reset();
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < resets; i++) env.reset();
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.0f", (double)resets * n / secs);
      fflush(out);
    }
    fprintf(out, "\n");
  }

  fprintf(out, "\n=== snapshot-reset throughput (env-resets/s, resetFromSnapshot = arena memcpy) ===\n");
  fprintf(out, "%-8s", "envs\\thr");
  for (auto t : threadCounts) fprintf(out, "%12u", t);
  fprintf(out, "\n");
  for (uint32_t n : envCounts) {
    fprintf(out, "%-8u", n);
    for (auto t : threadCounts) {
      uint32_t nt = (n == 1) ? 1u : std::min(t, n);
      VectorizedEnv env(gdy, n, nt);
      env.loadLevel(level);
      env.seed(1234);
      // step a bit so the snapshot differs from current state, then time
      // resetFromSnapshot() on each env directly
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});
      for (int i = 0; i < 10; i++) env.step(actions);
      int iters = resets * 4;
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; i++) {
        env.resetFromSnapshotAll();  // parallel over the pool
      }
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      fprintf(out, "%12.0f", (double)iters * n / secs);
      fflush(out);
    }
    fprintf(out, "\n");
  }

  {
    VectorizedEnv env(gdy, 1, 1);
    env.loadLevel(level);
    env.seed(1234);
    std::vector<std::vector<std::vector<int32_t>>> actions(1, {{1}});
    for (int i = 0; i < 20; i++) env.step(actions);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < steps; i++) {
      actions[0][0][0] = 1 + (int32_t)(rng() % nActions);
      env.step(actions);
    }
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    fprintf(out, "\nsingle env (1 env, 1 thread): %.0f steps/s (%.2f us/step)\n", steps / secs, secs * 1e6 / steps);
  }
  fclose(out);
  printf("done\n");
  return 0;
}
