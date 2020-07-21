#include "TurnBasedGameProcess.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(
    std::shared_ptr<Grid> grid,
    std::shared_ptr<Observer> observer,
    std::shared_ptr<GDYFactory> gdyFactory)
    : GameProcess(grid, observer, gdyFactory) {
}

TurnBasedGameProcess::~TurnBasedGameProcess() {
}

ActionResult TurnBasedGameProcess::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  spdlog::debug("Performing turn based actions for player {0}", playerId);
  auto rewards = grid_->performActions(playerId, actions);

  spdlog::debug("Updating Grid");
  auto delayedRewards = grid_->update();

  for (auto delayedReward : delayedRewards) {
    auto playerId = delayedReward.first;
    auto reward = delayedReward.second;
    delayedRewards_[playerId] += reward;
  }

  auto terminationResult = terminationHandler_->isTerminated();

  auto episodeComplete = terminationResult.terminated;

  if (episodeComplete) {
    reset();
  }

  if (delayedRewards_[playerId] > 0) {
    rewards.push_back(delayedRewards_[playerId]);
  }

  // reset reward for this player as they are being returned here
  delayedRewards_[playerId] = 0;

  return {terminationResult.playerStates, episodeComplete, rewards};
}  // namespace griddly

// This is only used in tests
void TurnBasedGameProcess::setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler) {
  terminationHandler_ = terminationHandler;
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

}  // namespace griddly