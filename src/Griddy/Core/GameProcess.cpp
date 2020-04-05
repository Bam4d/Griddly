#include "GameProcess.hpp"

#include <spdlog/spdlog.h>

#include "Players/Player.hpp"

namespace griddy {

GameProcess::GameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer) : grid_(grid), observer_(observer) {
}

GameProcess::~GameProcess() {}

void GameProcess::addPlayer(std::shared_ptr<Player> player) {
  spdlog::debug("Adding player Name={0}, Id={1}", player->getName(), player->getId());
  players_.push_back(player);
}

void GameProcess::init() {
  spdlog::debug("Initializing GameProcess {0}", getProcessName());
  if (observer_ != nullptr) {
    observer_->init(grid_->getWidth(), grid_->getHeight());
  }
}

void GameProcess::startGame() {
  for (auto &p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());
    p->init(grid_->getWidth(), grid_->getHeight(), shared_from_this());
  }

  isStarted_ = true;
}

void GameProcess::endGame() {
  isStarted_ = false;
}

bool GameProcess::isStarted() const {
  return isStarted_;
}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

std::unique_ptr<uint8_t[]> GameProcess::observe(int playerId) const {
  if (observer_ == nullptr) {
    return nullptr;
  }

  spdlog::debug("Generating observations for player {0}", playerId);

  return observer_->observe(playerId);
}

std::shared_ptr<Grid> GameProcess::getGrid() {
  return grid_;
}

std::shared_ptr<Observer> GameProcess::getObserver() {
  return observer_;
}

}  // namespace griddy