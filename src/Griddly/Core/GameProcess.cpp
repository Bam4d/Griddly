#include "GameProcess.hpp"

#include <spdlog/spdlog.h>

#include "GDY/Actions/Action.hpp"
#include "Players/Player.hpp"

namespace griddly {

GameProcess::GameProcess(
    ObserverType globalObserverType,
    std::shared_ptr<GDYFactory> gdyFactory,
    std::shared_ptr<Grid> grid)
    : grid_(grid), globalObserverType_(globalObserverType), gdyFactory_(gdyFactory) {
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

void GameProcess::setLevel(uint32_t levelId) {
  levelGenerator_ = gdyFactory_->getLevelGenerator(levelId);
}

void GameProcess::setLevel(std::string levelString) {
  levelGenerator_ = gdyFactory_->getLevelGenerator(levelString);
}

void GameProcess::setLevelGenerator(std::shared_ptr<LevelGenerator> levelGenerator) {
  levelGenerator_ = levelGenerator;
}

std::shared_ptr<LevelGenerator> GameProcess::getLevelGenerator() const {
  return levelGenerator_;
}

void GameProcess::init(bool isCloned) {
  if (isInitialized_) {
    throw std::runtime_error("Cannot re-initialize game process");
  }

  auto playerCount = gdyFactory_->getPlayerCount();

  if (!isCloned) {
    spdlog::debug("Initializing GameProcess {0}", getProcessName());

    if (levelGenerator_ == nullptr) {
      spdlog::info("No level specified, will use the first level described in the GDY.");
      setLevel(0);
    }

    grid_->setPlayerCount(playerCount);

    grid_->resetGlobalVariables(gdyFactory_->getGlobalVariableDefinitions());

    levelGenerator_->reset(grid_);

  } else {
    spdlog::debug("Initializing Cloned GameProcess {0}", getProcessName());
    requiresReset_ = false;
  }

  auto playerAvatarObjects = grid_->getPlayerAvatarObjects();

  // Global observer
  if (globalObserverType_ != ObserverType::NONE) {
    observer_ = gdyFactory_->createObserver(grid_, globalObserverType_);

    ObserverConfig globalObserverConfig = getObserverConfig(observer_->getObserverType());
    globalObserverConfig.gridXOffset = 0;
    globalObserverConfig.gridYOffset = 0;
    globalObserverConfig.playerId = 0;
    globalObserverConfig.playerCount = playerCount;
    globalObserverConfig.highlightPlayers = true;
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

  for (auto& p : players_) {
    spdlog::debug("Initializing player Name={0}, Id={1}", p->getName(), p->getId());

    ObserverConfig observerConfig = getObserverConfig(p->getObserver()->getObserverType());
    observerConfig.overrideGridHeight = playerObserverDefinition.gridHeight;
    observerConfig.overrideGridWidth = playerObserverDefinition.gridWidth;
    observerConfig.gridXOffset = playerObserverDefinition.gridXOffset;
    observerConfig.gridYOffset = playerObserverDefinition.gridYOffset;
    observerConfig.rotateWithAvatar = playerObserverDefinition.rotateWithAvatar;
    observerConfig.playerId = p->getId();
    observerConfig.playerCount = playerObserverDefinition.playerCount;
    observerConfig.highlightPlayers = playerObserverDefinition.highlightPlayers;

    p->init(observerConfig, playerObserverDefinition.trackAvatar, shared_from_this());

    if (playerAvatarObjects.size() > 0) {
      auto playerId = p->getId();
      if (playerAvatarObjects.find(playerId) == playerAvatarObjects.end()) {
        std::string errorMessage = fmt::format("Cannot find avatar for player {0}. Make sure an avatar for this player is defined in the level_string e.g 'A{0}'", playerId);
        spdlog::error(errorMessage);
        throw std::invalid_argument(errorMessage);
      }
      p->setAvatar(playerAvatarObjects.at(p->getId()));
    }
  }

  terminationHandler_ = gdyFactory_->createTerminationHandler(grid_, players_);

  // if the environment is cloned, it will not be reset before being used, so make sure the observers are reset
  if (isCloned) {
    resetObservers();
  }

  isInitialized_ = true;
}

void GameProcess::resetObservers() {
  auto playerAvatarObjects = grid_->getPlayerAvatarObjects();

  for (auto& p : players_) {
    p->reset();
    spdlog::debug("{0} player avatar objects to reset", playerAvatarObjects.size());
    if (playerAvatarObjects.size() > 0) {
      p->setAvatar(playerAvatarObjects.at(p->getId()));
    }
  }

  if (observer_ != nullptr) {
    observer_->reset();
  }
}

void GameProcess::reset() {
  if (!isInitialized_) {
    throw std::runtime_error("Cannot reset game process before initialization.");
  }

  spdlog::debug("Resetting player count.");
  grid_->setPlayerCount(gdyFactory_->getPlayerCount());

  spdlog::debug("Resetting global variables.");
  grid_->resetGlobalVariables(gdyFactory_->getGlobalVariableDefinitions());

  spdlog::debug("Resetting level generator.");
  levelGenerator_->reset(grid_);

  spdlog::debug("Resetting Observers.");
  resetObservers();

  spdlog::debug("Resetting Termination Handler.");
  terminationHandler_ = std::shared_ptr<TerminationHandler>(gdyFactory_->createTerminationHandler(grid_, players_));

  requiresReset_ = false;
  spdlog::debug("Reset Complete.");
}

ObserverConfig GameProcess::getObserverConfig(ObserverType observerType) const {
  switch (observerType) {
    case ObserverType::ISOMETRIC:
      return gdyFactory_->getIsometricSpriteObserverConfig();
    case ObserverType::SPRITE_2D:
      return gdyFactory_->getSpriteObserverConfig();
    case ObserverType::BLOCK_2D:
      return gdyFactory_->getBlockObserverConfig();
    case ObserverType::VECTOR:
      return gdyFactory_->getVectorObserverConfig();
    default:
      return ObserverConfig{};
  }
}

void GameProcess::release() {
  spdlog::warn("Forcing release of vulkan");
  observer_->release();
  for (auto& p : players_) {
    p->getObserver()->release();
  }
}

bool GameProcess::isInitialized() {
  return isInitialized_;
}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

uint32_t GameProcess::getNumPlayers() const {
  return players_.size();
}

uint8_t* GameProcess::observe() const {
  // if (observer_ == nullptr) {
  //   return nullptr;
  // }

  return observer_->update();
}

std::shared_ptr<Grid> GameProcess::getGrid() {
  return grid_;
}

std::shared_ptr<Observer> GameProcess::getObserver() {
  return observer_;
}

int32_t GameProcess::getAccumulatedRewards(uint32_t playerId) {
  int32_t reward = accumulatedRewards_[playerId];
  accumulatedRewards_[playerId] = 0;
  return reward;
}

std::unordered_map<glm::ivec2, std::unordered_set<std::string>> GameProcess::getAvailableActionNames(uint32_t playerId) const {
  std::unordered_map<glm::ivec2, std::unordered_set<std::string>> availableActionNames;

  // TODO: we can cache a lot of this if there are many players so it only needs to be created once.
  std::unordered_set<std::string> internalActions;
  auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
  for (auto actionInputDefinition : actionInputsDefinitions) {
    if (actionInputDefinition.second.internal) {
      internalActions.insert(actionInputDefinition.first);
    }
  }

  // For every object in the grid return the actions that the object can perform
  // TODO: do not iterate over all the objects if we have avatars.
  for (auto object : grid_->getObjects()) {
    if (playerId == object->getPlayerId()) {
      auto actions = object->getAvailableActionNames();

      for (auto internalActionName : internalActions) {
        actions.erase(internalActionName);
      }

      auto location = object->getLocation();
      if (actions.size() > 0) {
        availableActionNames.insert({location, actions});
      }
    }
  }

  return availableActionNames;
}

std::vector<uint32_t> GameProcess::getAvailableActionIdsAtLocation(glm::ivec2 location, std::string actionName) const {
  auto srcObject = grid_->getObject(location);

  spdlog::debug("Getting available actionIds for action [{}] at location [{0},{1}]", actionName, location.x, location.y);

  std::vector<uint32_t> availableActionIds{};
  if (srcObject) {
    auto actionInputDefinitions = gdyFactory_->getActionInputsDefinitions();
    auto actionInputDefinition = actionInputDefinitions[actionName];

    auto relativeToSource = actionInputDefinition.relative;

    for (auto inputMapping : actionInputDefinition.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      auto metaData = mapping.metaData;

      // Create an fake action to test for availability (and not duplicate a bunch of code)
      auto potentialAction = std::shared_ptr<Action>(new Action(grid_, actionName, 0, 0, metaData));
      potentialAction->init(srcObject, mapping.vectorToDest, mapping.orientationVector, relativeToSource);

      if (srcObject->isValidAction(potentialAction)) {
        availableActionIds.push_back(actionId);
      }
    }
  }

  return availableActionIds;
}

void GameProcess::generateStateHash(StateInfo& stateInfo) const {

  // Hash global variables
  for (auto variableIt : stateInfo.globalVariables) {
    hash_combine(stateInfo.hash, variableIt.first);
    for (auto playerVariableIt : variableIt.second) {
      hash_combine(stateInfo.hash, playerVariableIt.second);
      hash_combine(stateInfo.hash, playerVariableIt.first);
    }
  }

  // Hash ordered object list
  std::sort(stateInfo.objectInfo.begin(), stateInfo.objectInfo.end(), SortObjectInfo());
  for(auto o : stateInfo.objectInfo) {
    hash_combine(stateInfo.hash, o.name);
    hash_combine(stateInfo.hash, o.location);
    hash_combine(stateInfo.hash, o.orientation.getUnitVector());
    hash_combine(stateInfo.hash, o.playerId);

    // Hash the object variables
    for(auto variableIt : o.variables) {
      hash_combine(stateInfo.hash, variableIt.first);
      hash_combine(stateInfo.hash, variableIt.second);
    }
  }
}

StateInfo GameProcess::getState() const {
  StateInfo stateInfo;

  stateInfo.gameTicks = *grid_->getTickCount();

  auto& globalVariables = grid_->getGlobalVariables();

  for (auto& globalVarIt : globalVariables) {
    auto variableName = globalVarIt.first;
    auto variableValues = globalVarIt.second;
    for (auto variableValue : variableValues) {
      stateInfo.globalVariables[variableName].insert({variableValue.first, *variableValue.second});
    }
  }

  for (auto& object : grid_->getObjects()) {
    ObjectInfo objectInfo;

    objectInfo.name = object->getObjectName();
    objectInfo.location = object->getLocation();
    objectInfo.playerId = object->getPlayerId();
    objectInfo.orientation = object->getObjectOrientation();

    for (auto varIt : object->getAvailableVariables()) {
      if (globalVariables.find(varIt.first) == globalVariables.end()) {
        objectInfo.variables.insert({varIt.first, *varIt.second});
      }
    }

    stateInfo.objectInfo.push_back(objectInfo);
  }

  generateStateHash(stateInfo);

  return stateInfo;
}

}  // namespace griddly