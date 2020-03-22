#include "GameProcess.hpp"
#include "Players/Player.hpp"

#include <spdlog/spdlog.h>

namespace griddy {

GameProcess::GameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid) : grid_(grid), players_(players), observer_(observer) {
  spdlog::debug("Creating GameProcess {0}", getProcessName());
}

GameProcess::~GameProcess() {}

void GameProcess::init() {
  
  for (auto const& p : players_) {
    spdlog::debug("Player Name={0}, Id={1}", p->getName(), p->getId());
  }

  observer_->init(grid_->getWidth(), grid_->getHeight());

}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

std::unique_ptr<uint8_t[]> GameProcess::observe(int playerId) const {
  return observer_->observe(playerId, grid_);
}

}  // namespace griddy