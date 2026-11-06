// single env, many frames, measure pure GPU draw throughput
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"
#include "Griddly/Core/Observers/SpriteObserver.hpp"
#include <SDL3/SDL.h>
using namespace griddly;
int main() {
  std::string root = "/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources";
  ResourceConfig rc{root+"/games", root+"/images", ""};
  auto og = std::make_shared<ObjectGenerator>();
  auto tg = std::make_shared<TerminationGenerator>();
  auto gdy = std::make_shared<GDYFactory>(og, tg, rc);
  gdy->initializeFromFile(root+"/games/Single-Player/Mini-Grid/minigrid-spiders.yaml");
  VectorizedEnv env(gdy, 1, 1);
  env.init(); env.loadLevel(4); env.seed(1);
  env.step({{ {1} }});
  auto obs = std::static_pointer_cast<SpriteObserver>(gdy->createObserver(env.grid(0), "Sprite2D", 1, 0));
  obs->init({}); obs->reset();
  obs->update();  // first render inits SDL
  printf("gpu=%d\n", obs->isUsingGPU());
  auto t0 = std::chrono::steady_clock::now();
  int N = 2000;
  for (int i = 0; i < N; i++) obs->update();
  auto t1 = std::chrono::steady_clock::now();
  double secs = std::chrono::duration<double>(t1-t0).count();
  printf("single-env GPU update (with readback): %.1f fps (%.3f ms/frame)\n", N/secs, secs*1000/N);
  return 0;
}
