#include "TurnBasedGameProcess.hpp"

namespace griddy {

const std::string TurnBasedGameProcess::name = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid)
    : GameProcess(players, observer, grid)
{
}

TurnBasedGameProcess::~TurnBasedGameProcess() {}

void TurnBasedGameProcess::startGame() const {

}

void TurnBasedGameProcess::endGame() const {
  
}

int TurnBasedGameProcess::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  grid_->performActions(playerId, actions)
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name;
}

}  // namespace griddy