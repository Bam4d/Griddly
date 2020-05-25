#include "GameProcess.hpp"

#include <spdlog/spdlog.h>

#include "Players/Player.hpp"

namespace griddly {

GameProcess::GameProcess(
    std::shared_ptr<Grid> grid,
    std::shared_ptr<Observer> observer,
    std::shared_ptr<GDYFactory> gdyFactory)
    : grid_(grid), observer_(observer), gdyFactory_(gdyFactory) {
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

  auto levelGenerator = gdyFactory_->getLevelGenerator();

  grid_->resetGlobalParameters(gdyFactory_->getGlobalParameterDefinitions());

  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars;
  if (levelGenerator != nullptr) {
    playerAvatars = levelGenerator->reset(grid_);
  }

  if (observer_ != nullptr) {
    observer_->init(grid_->getWidth(), grid_->getHeight());
  }

  for (auto &p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());
    p->init(grid_->getWidth(), grid_->getHeight(), shared_from_this());
    if (gdyFactory_->getActionControlMode() == ActionControlMode::DIRECT) {
      p->setAvatar(playerAvatars.at(p->getId()));
    }
  }

  terminationHandler_ = std::shared_ptr<TerminationHandler>(gdyFactory_->createTerminationHandler(grid_, players_));

  isInitialized_ = true;
}

std::unique_ptr<uint8_t[]> GameProcess::reset() {
  if (!isInitialized_) {
    throw std::runtime_error("Cannot reset game process before initialization.");
  }

  auto levelGenerator = gdyFactory_->getLevelGenerator();

  grid_->resetGlobalParameters(gdyFactory_->getGlobalParameterDefinitions());

  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars;
  if (levelGenerator != nullptr) {
    playerAvatars = levelGenerator->reset(grid_);
  }

  std::unique_ptr<uint8_t[]> observation;
  if (observer_ != nullptr) {
    observation = observer_->reset();
  } else {
    observation = nullptr;
  }

  for (auto &p : players_) {
    p->reset();
    if (gdyFactory_->getActionControlMode() == ActionControlMode::DIRECT) {
      p->setAvatar(playerAvatars.at(p->getId()));
    }
  }

  terminationHandler_ = std::shared_ptr<TerminationHandler>(gdyFactory_->createTerminationHandler(grid_, players_));

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

}  // namespace griddly