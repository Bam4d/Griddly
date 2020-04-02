#include "TurnBasedGameProcess.hpp"

namespace griddy {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid)
    : GameProcess(players, observer, grid)
{
}

TurnBasedGameProcess::~TurnBasedGameProcess() {}

std::vector<int> TurnBasedGameProcess::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  auto rewards = grid_->performActions(playerId, actions);
  grid_->update();
  return rewards;
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

}  // namespace griddy