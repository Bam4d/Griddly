// Benchmark: VectorizedEnv parallel step FPS vs numEnvs/threads, and vs a
// single sequential env.
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
  std::string game = argc > 1 ? argv[1] : "Single-Player/GVGAI/bait.yaml";
  uint32_t level = argc > 2 ? std::stoul(argv[2]) : 0;
  int steps = argc > 3 ? std::atoi(argv[3]) : 2000;

  auto gdy = loadGdy(root, game);
  gdy->setMaxSteps(1000000);
  auto actionNames = gdy->getExternalActionNames();
  uint32_t nActions = 1;
  for (auto& name : actionNames) nActions = std::max(nActions, (uint32_t)gdy->findActionInputsDefinition(name).inputMappings.size());

  std::mt19937 rng(42);

  // sequential baseline: 1 env, stepped on the calling thread
  auto bench = [&](uint32_t numEnvs, uint32_t threads, bool parallel) {
    VectorizedEnv env(gdy, numEnvs, parallel ? threads : 1);
    env.loadLevel(level);
    env.seed(1234);

    std::vector<std::vector<std::vector<int32_t>>> actions(numEnvs);
    for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});

    // warmup
    for (int i = 0; i < 50; i++) env.step(actions);

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < steps; i++) {
      for (auto& a : actions) a[0][0] = 1 + (int32_t)(rng() % nActions);
      env.step(actions);
    }
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    double envStepsPerSec = (double)steps * numEnvs / secs;
    printf("envs=%3u threads=%2u parallel=%d  %8.3fs  %10.0f env-steps/s  (%6.1f us/env-step)\n",
           numEnvs, threads, (int)parallel, secs, envStepsPerSec, secs * 1e6 / (steps * numEnvs));
    fflush(stdout);
  };

  printf("game=%s level=%u steps=%d\n", game.c_str(), level, steps);
  printf("%-8s", "envs\\threads");
  std::vector<uint32_t> threadCounts = {1u, 4u, 8u, 16u, 32u};
  for (auto t : threadCounts) printf("%10u", t);
  printf("\n");
  for (uint32_t n : {1u, 16u, 64u, 128u, 256u, 512u}) {
    printf("%-8u", n);
    for (auto t : threadCounts) {
      // capture just the env-steps/s number
      uint32_t nt = (n == 1) ? 1u : t;
      VectorizedEnv env(gdy, n, nt);
      env.loadLevel(level);
      env.seed(1234);
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng() % nActions)});
      for (int i = 0; i < 50; i++) env.step(actions);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < steps; i++) {
        for (auto& a : actions) a[0][0] = 1 + (int32_t)(rng() % nActions);
        env.step(actions);
      }
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1 - t0).count();
      printf("%10.0f", (double)steps * n / secs);
      fflush(stdout);
    }
    printf("\n");
  }
  return 0;
}
