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
  if(isInitialized_) {
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

void GameProcess::reset() {
  if(!isInitialized_) {
    throw std::runtime_error("Cannot reset game process before initialization.");
  }

  if (levelGenerator_ != nullptr) {
    levelGenerator_->reset(grid_);
  }
}

void GameProcess::startGame() {
  if(!isInitialized_) {
    throw std::runtime_error("Cannot start game before initialization.");
  }

  isStarted_ = true;
}

void GameProcess::endGame() {
  if(!isInitialized_) {
    throw std::runtime_error("Cannot end game before initialization.");
  }

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

std::shared_ptr<LevelGenerator> GameProcess::getLevelGenerator() {
  return levelGenerator_;
}

}  // namespace griddy