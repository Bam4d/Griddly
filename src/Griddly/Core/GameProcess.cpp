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

  if (players_.size() < gdyFactory_->getPlayerCount()) {
    players_.push_back(player);
  } else {
    auto errorString = fmt::format("The {0} environment can only support {1} players.", gdyFactory_->getName(), gdyFactory_->getPlayerCount());
    throw std::invalid_argument(errorString);
  }
}

void GameProcess::init() {
  if (isInitialized_) {
    throw std::runtime_error("Cannot re-initialize game process");
  }

  spdlog::debug("Initializing GameProcess {0}", getProcessName());

  auto levelGenerator = gdyFactory_->getLevelGenerator();
  auto playerCount = gdyFactory_->getPlayerCount();

  grid_->resetGlobalVariables(gdyFactory_->getGlobalVariableDefinitions());

  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars;
  if (levelGenerator != nullptr) {
    playerAvatars = levelGenerator->reset(grid_);
  }

  // Global observer
  if (observer_ != nullptr) {
    ObserverConfig globalObserverConfig;
    globalObserverConfig.gridXOffset = 0;
    globalObserverConfig.gridYOffset = 0;
    globalObserverConfig.playerId = 0;
    globalObserverConfig.playerCount = playerCount;
    observer_->init(globalObserverConfig);
  }

  auto playerObserverDefinition = gdyFactory_->getPlayerObserverDefinition();
  if (playerObserverDefinition.gridHeight == 0 || playerObserverDefinition.gridWidth == 0) {
    spdlog::debug("Using Default player observation definition");
    playerObserverDefinition.trackAvatar = false;
    playerObserverDefinition.playerCount = playerCount;
  }

  // Check that the number of registered players matches the count for the environment
  if (players_.size() != playerCount) {
    auto errorString = fmt::format("The \"{0}\" environment requires {1} player(s), but {2} have been registered.", gdyFactory_->getName(), gdyFactory_->getPlayerCount(), players_.size());
    throw std::invalid_argument(errorString);
  }

  for (auto &p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());
    p->init(playerObserverDefinition, shared_from_this());

    if (playerAvatars.size() > 0) {
      p->setAvatar(playerAvatars.at(p->getId()));
    }
  }

  terminationHandler_ = gdyFactory_->createTerminationHandler(grid_, players_);

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
    if (playerAvatars.size() > 0) {
      p->setAvatar(playerAvatars.at(p->getId()));
    }
  }

  terminationHandler_ = std::shared_ptr<TerminationHandler>(gdyFactory_->createTerminationHandler(grid_, players_));

  isStarted_ = true;

  return observation;
}

void GameProcess::release() {
  spdlog::warn("Forcing release of vulkan");
  observer_->release();
  for (auto &p : players_) {
    p->getObserver()->release();
  }
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