#include "GameProcess.hpp"
#include "Players/Player.hpp"

#include <spdlog/spdlog.h>

namespace griddy {

GameProcess::GameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid) : grid_(grid), players_(players), observer_(observer) {
  spdlog::debug("Creating GameProcess {0}", getProcessName());
  spdlog::debug("Adding {0} players: ", players.size());

  for (auto const& p : players) {
    spdlog::debug("Player Name={0}, Id={1}", p->getName(), p->getId());
  }

  observer->init(grid->getWidth(), grid->getHeight());
}

GameProcess::~GameProcess() {}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

std::unique_ptr<uint8_t[]> GameProcess::observe(int playerId) const {
  return observer_->observe(playerId, grid_);
}

}  // namespace griddy