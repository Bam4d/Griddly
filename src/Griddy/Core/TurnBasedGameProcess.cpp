#include "TurnBasedGameProcess.hpp"
#include<spdlog/spdlog.h>

namespace griddy {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<LevelGenerator> levelGenerator)
    : GameProcess(grid, observer, levelGenerator) {
}

TurnBasedGameProcess::~TurnBasedGameProcess() {
}

std::vector<int> TurnBasedGameProcess::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  spdlog::debug("Performing turn based actions for player {0}", playerId);
  auto rewards = grid_->performActions(playerId, actions);

  spdlog::debug("Updating Grid");
  grid_->update();
  return rewards;
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

}  // namespace griddy