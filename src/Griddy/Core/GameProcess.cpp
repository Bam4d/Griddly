#include "GameProcess.hpp"

#include <spdlog/spdlog.h>

#include "Players/Player.hpp"

namespace griddy {

GameProcess::GameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<LevelGenerator> levelGenerator) : grid_(grid), observer_(observer), levelGenerator_(levelGenerator) {
}

GameProcess::~GameProcess() {}

void GameProcess::addPlayer(std::shared_ptr<Player> player) {
  spdlog::debug("Adding player Name={0}, Id={1}", player->getName(), player->getId());
  players_.push_back(player);
}

void GameProcess::init() {
  if (isInitialized_) {
    throw std::runtime_error("Cannot re-initialize game process");
  }

  spdlog::debug("Initializing GameProcess {0}", getProcessName());

  if (levelGenerator_ != nullptr) {
    levelGenerator_->reset(grid_);
  }

  if (observer_ != nullptr) {
    observer_->init(grid_->getWidth(), grid_->getHeight());
  }

  for (auto &p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());
    p->init(grid_->getWidth(), grid_->getHeight(), shared_from_this());
  }

  isInitialized_ = true;
}

std::unique_ptr<uint8_t[]> GameProcess::reset() {
  if (!isInitialized_) {
    throw std::runtime_error("Cannot reset game process before initialization.");
  }

  if (levelGenerator_ != nullptr) {
    levelGenerator_->reset(grid_);
  }

  std::unique_ptr<uint8_t[]> observation;
  if (observer_ != nullptr) {
    observation = observer_->reset();
  } else {
    observation = nullptr;
  }

  for (auto &p : players_) {
    p->reset();
  }

  isStarted_ = true;

  return observation;
}

bool GameProcess::isStarted() {
  return isStarted_;
}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

std::unique_ptr<uint8_t[]> GameProcess::observe(uint32_t playerId) const {
  if (observer_ == nullptr) {
    return nullptr;
  }

  spdlog::debug("Generating observations for player {0}", playerId);

  return observer_->update(playerId);
}

std::shared_ptr<Grid> GameProcess::getGrid() {
  return grid_;
}

std::shared_ptr<Observer> GameProcess::getObserver() {
  return observer_;
}

std::shared_ptr<LevelGenerator> GameProcess::getLevelGenerator() {
  return levelGenerator_;
}

}  // namespace griddy