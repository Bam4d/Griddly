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

std::string TurnBasedGameProcess::getProcessName() const {
  return name;
}

}  // namespace griddy