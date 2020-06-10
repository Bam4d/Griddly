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

  grid_->resetGlobalVariables(gdyFactory_->getGlobalVariableDefinitions());

  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars;
  if (levelGenerator != nullptr) {
    playerAvatars = levelGenerator->reset(grid_);
  }

  // Global observer
  if (observer_ != nullptr) {
    ObserverConfig globalObserverConfig;
    globalObserverConfig.gridHeight = grid_->getHeight();
    globalObserverConfig.gridWidth = grid_->getWidth();
    globalObserverConfig.gridXOffset = 0;
    globalObserverConfig.gridYOffset = 0;
    observer_->init(globalObserverConfig);
  }

  auto playerObserverDefinition = gdyFactory_->getPlayerObserverDefinition();
  if(playerObserverDefinition.gridHeight == 0 || playerObserverDefinition.gridWidth == 0) {
    spdlog::debug("Using Default player observation definition");
    playerObserverDefinition.gridHeight = grid_->getHeight();
    playerObserverDefinition.gridWidth = grid_->getWidth();
    playerObserverDefinition.trackAvatar = false;
  }

  for (auto &p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());
    p->init(playerObserverDefinition, shared_from_this());

    auto playerAvatar = gdyFactory_->getActionControlMode() == ActionControlMode::DIRECT ? playerAvatars.at(p->getId()) : nullptr;
    p->setAvatar(playerAvatar);
  }

  terminationHandler_ = std::shared_ptr<TerminationHandler>(gdyFactory_->createTerminationHandler(grid_, players_));

  isInitialized_ = true;
}

std::shared_ptr<uint8_t> GameProcess::reset() {
  if (!isInitialized_) {
    throw std::runtime_error("Cannot reset game process before initialization.");
  }

  auto levelGenerator = gdyFactory_->getLevelGenerator();

  grid_->resetGlobalVariables(gdyFactory_->getGlobalVariableDefinitions());

  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars;
  if (levelGenerator != nullptr) {
    playerAvatars = levelGenerator->reset(grid_);
  }

  std::shared_ptr<uint8_t> observation;
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

uint32_t GameProcess::getNumPlayers() const {
  return players_.size();
}

std::shared_ptr<uint8_t> GameProcess::observe(uint32_t playerId) const {
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