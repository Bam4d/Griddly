// Bare single-env tight loop: no VectorizedEnv, no thread pool.
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/TurnBasedGameProcess.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "Griddly/Core/GDY/Actions/Action.hpp"

using namespace griddly;

int main(int argc, char** argv) {
  std::string root = "/mnt/vast/home/bam4d/.vibe/worktrees/Griddly-c5a12f524638/vibe_main/resources";
  std::string game = argc > 1 ? argv[1] : "Single-Player/GVGAI/bait.yaml";
  uint32_t level = argc > 2 ? std::stoul(argv[2]) : 0;
  int steps = argc > 3 ? std::atoi(argv[3]) : 2000;

  ResourceConfig rc{root + "/games", root + "/images", ""};
  auto og = std::make_shared<ObjectGenerator>();
  auto tg = std::make_shared<TerminationGenerator>();
  auto gdy = std::make_shared<GDYFactory>(og, tg, rc);
  gdy->initializeFromFile(root + "/games/" + game);
  gdy->setMaxSteps(1000000);

  auto grid = std::make_shared<Grid>();
  auto gp = std::make_shared<TurnBasedGameProcess>("None", gdy, grid);
  auto player = std::make_shared<Player>(1, "Player 1", "None", gp);
  gp->addPlayer(player);
  gp->setLevel(level);
  gp->init(false);
  gp->reset();
  gp->seedRandomGenerator(1234);

  auto names = gdy->getExternalActionNames();
  uint32_t nActions = 1;
  for (auto& n : names) nActions = std::max(nActions, (uint32_t)gdy->findActionInputsDefinition(n).inputMappings.size());

  std::mt19937 rng(42);
  auto avatar = player->getAvatar();

  int mode = argc > 4 ? std::atoi(argv[4]) : 0;  // 0=valid action, 1=noop (empty), 2=valid but updateTicks=false
  auto t0 = std::chrono::steady_clock::now();
  int32_t total = 0;
  for (int i = 0; i < steps; i++) {
    std::vector<Action*> acts;
    if (mode != 1) {
      auto actionId = (int32_t)(1 + rng() % nActions);
      const auto& def = gdy->findActionInputsDefinition(names[0]);
      const auto& mapping = def.inputMappings.at(actionId);
      auto* action = grid->acquireAction(names[0], 1);
      action->init(avatar, mapping.vectorToDest, mapping.orientationVector, def.relative);
      acts.push_back(action);
    }
    auto res = gp->performActionsPooled(1, acts, mode != 2);
    total += gp->getAccumulatedRewards(1);
    if (res.terminated) gp->resetFromSnapshot();
  }
  auto t1 = std::chrono::steady_clock::now();
  double secs = std::chrono::duration<double>(t1 - t0).count();
  printf("bare single env: %d steps in %.3fs = %.0f steps/s (%.2f us/step), reward=%d\n",
         steps, secs, steps / secs, secs * 1e6 / steps, total);
  return 0;
}
