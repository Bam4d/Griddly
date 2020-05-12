#include "TurnBasedGameProcess.hpp"

#include <spdlog/spdlog.h>

namespace griddy {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(
    std::shared_ptr<Grid> grid,
    std::shared_ptr<Observer> observer,
    std::shared_ptr<LevelGenerator> levelGenerator,
    std::shared_ptr<TerminationHandler> terminationHandler)
    : GameProcess(grid, observer, levelGenerator, terminationHandler) {
}

TurnBasedGameProcess::~TurnBasedGameProcess() {
}

std::vector<int> TurnBasedGameProcess::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  spdlog::debug("Performing turn based actions for player {0}", playerId);
  auto rewards = grid_->performActions(playerId, actions);

  spdlog::debug("Updating Grid");
  grid_->update();

  auto terminationResult = terminationHandler_->isTerminated();

  if (terminationResult.terminated) {
    reset();
  }

  return rewards;
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

}  // namespace griddy