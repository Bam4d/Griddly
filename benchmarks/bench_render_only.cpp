// Render-only FPS sweep on tiled renderer, envs x threads. Usage: bench_render_only [steps]
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"
#include "Griddly/Core/Observers/TiledSDLRenderer.hpp"
using namespace griddly;
int main(int argc, char** argv) {
  int frames = argc > 1 ? atoi(argv[1]) : 100;
  std::string root = "/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources";
  ResourceConfig rc{root+"/games", root+"/images", ""};
  auto og = std::make_shared<ObjectGenerator>();
  auto tg = std::make_shared<TerminationGenerator>();
  auto gdy = std::make_shared<GDYFactory>(og, tg, rc);
  gdy->initializeFromFile(root+"/games/Single-Player/Mini-Grid/minigrid-spiders.yaml");
  std::mt19937 rng(42);
  uint32_t nActions = 4;
  printf("%-6s","envs");
  for (uint32_t t : {1u,4u,8u,16u,32u}) printf("%10u", t);
  printf("\n");
  for (uint32_t n : {1u,4u,8u,16u,32u,64u,128u,256u}) {
    printf("%-6u", n);
    for (uint32_t t : {1u,4u,8u,16u,32u}) {
      uint32_t nt = n==1 ? 1u : std::min(t, n);
      VectorizedEnv env(gdy, n, nt);
      env.loadLevel(4); env.seed(1234);
      std::vector<std::vector<std::vector<int32_t>>> actions(n);
      for (auto& a : actions) a.push_back({(int32_t)(1 + rng()%nActions)});
      env.step(actions);
      std::vector<std::shared_ptr<Grid>> grids;
      for (uint32_t i=0;i<n;i++) grids.push_back(env.grid(i));
      TiledSDLRendererConfig cfg;
      TiledSDLRenderer r(gdy, n, grids, cfg);
      r.render(true, nt);  // warmup
      auto t0 = std::chrono::steady_clock::now();
      int fr = std::min(frames, 300);
      for (int i=0;i<fr;i++) r.render(true, nt);
      auto t1 = std::chrono::steady_clock::now();
      double secs = std::chrono::duration<double>(t1-t0).count();
      // pixel checksum so "drew nothing" can't look fast
      const uint8_t* buf = r.buffer().data();
      uint64_t sum = 0;
      for (size_t k = 0; k < r.buffer().size(); k += 997) sum += buf[k];
      printf("%10.1f (cksum %llu)", fr/secs, (unsigned long long)sum);
      fflush(stdout);
    }
    printf("\n");
  }
  return 0;
}
