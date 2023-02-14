#include <spdlog/spdlog.h>

#include <utility>

#include "GDY/Actions/Action.hpp"
#include "GameProcess.hpp"
#include "Players/Player.hpp"

namespace griddly {

GameProcess::GameProcess(
    std::string globalObserverName,
    std::shared_ptr<GDYFactory> gdyFactory,
    std::shared_ptr<Grid> grid)
    : grid_(std::move(grid)), globalObserverName_(globalObserverName), gdyFactory_(std::move(gdyFactory)) {
}

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
  levelGenerator_ = gdyFactory_->getLevelGenerator(std::move(levelString));
}

void GameProcess::setLevelGenerator(std::shared_ptr<LevelGenerator> levelGenerator) {
  levelGenerator_ = std::move(levelGenerator);
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

    spdlog::debug("Resetting level generator");
    levelGenerator_->reset(grid_);
    spdlog::debug("Reset.");

  } else {
    spdlog::debug("Initializing Cloned GameProcess {0}", getProcessName());
    requiresReset_ = false;
  }

  // Check that the number of registered players matches the count for the environment
  if (players_.size() != playerCount) {
    auto errorString = fmt::format("The \"{0}\" environment requires {1} player(s), but {2} have been registered.", gdyFactory_->getName(), gdyFactory_->getPlayerCount(), players_.size());
    throw std::invalid_argument(errorString);
  }

  std::vector<std::weak_ptr<Observer>> playerObservers;
  for (auto& p : players_) {
    const auto& observer = gdyFactory_->createObserver(grid_, p->getObserverName(), playerCount, p->getId());
    p->init(observer);
    playerObservers.push_back(observer);
  }

  // Global observer
  spdlog::debug("Creating global observer: {0}", globalObserverName_);
  observer_ = gdyFactory_->createObserver(grid_, globalObserverName_, playerCount);

  // Init all the observers
  for (auto& p : players_) {
    p->getObserver()->init(playerObservers);
  }
  observer_->init(playerObservers);

  // if the environment is cloned, it will not be reset before being used, so make sure the observers are reset
  if (isCloned) {
    terminationHandler_ = gdyFactory_->createTerminationHandler(grid_, players_);
    resetObservers();
  }

  isInitialized_ = true;
}

void GameProcess::resetObservers() {
  auto playerAvatarObjects = grid_->getPlayerAvatarObjects();

  // Player observer reset
  spdlog::debug("{0} player avatar objects to reset", playerAvatarObjects.size());
  for (auto& p : players_) {
    if (playerAvatarObjects.find(p->getId()) != playerAvatarObjects.end()) {
      const auto& avatarObject = playerAvatarObjects.at(p->getId());
      p->reset(avatarObject);
    } else {
      p->reset();
    }
  }

  // Global observer reset
  observer_->reset();
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
  terminationHandler_ = gdyFactory_->createTerminationHandler(grid_, players_);

  requiresReset_ = false;
  spdlog::debug("Reset Complete.");
}

void GameProcess::release() {
  observer_->release();
  for (auto& p : players_) {
    p->getObserver()->release();
  }

  players_.clear();

  grid_->reset();
}

bool GameProcess::isInitialized() const {
  return isInitialized_;
}

std::string GameProcess::getProcessName() const {
  return "Unknown";
}

uint32_t GameProcess::getNumPlayers() const {
  return static_cast<uint32_t>(players_.size());
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
  const auto& actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
  for (const auto& actionInputDefinition : actionInputsDefinitions) {
    if (actionInputDefinition.second.internal) {
      internalActions.insert(actionInputDefinition.first);
    }
  }

  // For every object in the grid return the actions that the object can perform
  // TODO: do not iterate over all the objects if we have avatars.
  for (const auto& object : grid_->getObjects()) {
    if (playerId == object->getPlayerId()) {
      auto actions = object->getAvailableActionNames();

      for (const auto& internalActionName : internalActions) {
        actions.erase(internalActionName);
      }

      auto location = object->getLocation();
      if (!actions.empty()) {
        availableActionNames.insert({location, actions});
      }
    }
  }

  return availableActionNames;
}

std::vector<uint32_t> GameProcess::getAvailableActionIdsAtLocation(glm::ivec2 location, std::string actionName) const {
  auto srcObject = grid_->getObject(location);

  spdlog::debug("Getting available actionIds for action [{0}] at location [{0},{1}]", actionName, location.x, location.y);

  std::vector<uint32_t> availableActionIds{};
  if (srcObject) {
    const auto& actionInputDefinitions = gdyFactory_->getActionInputsDefinitions();
    const auto& actionInputDefinition = actionInputDefinitions.at(actionName);

    auto relativeToSource = actionInputDefinition.relative;

    for (const auto& inputMapping : actionInputDefinition.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      auto metaData = mapping.metaData;

      // Create an fake action to test for availability (and not duplicate a bunch of code)
      auto potentialAction = std::make_shared<Action>(Action(grid_, actionName, 0, 0, metaData));
      potentialAction->init(srcObject, mapping.vectorToDest, mapping.orientationVector, relativeToSource);

      if (srcObject->isValidAction(potentialAction)) {
        availableActionIds.push_back(actionId);
      }
    }
  }

  return availableActionIds;
}

void GameProcess::generateStateHash(GameState& gameState) {
  const auto& stateMapping = getGameStateMapping();

  const uint32_t stepsIdx = stateMapping.globalVariableNameToIdx.at("_steps");

  // Hash global variables
  for (uint32_t g = 0; g < gameState.globalData.size(); g++) {
    if (g != stepsIdx) {
      for (const auto& variableValue : gameState.globalData[g]) {
        hash_combine(gameState.hash, variableValue);
      }
    }
  }

  // TODO: this is a big wasteful but need to get consistent hashes regardless of OS and order of objects
  // auto objectDataCopy = std::vector<GameObjectData>(gameState.objectData.begin(), gameState.objectData.end());
  std::sort(gameState.objectData.begin(), gameState.objectData.end(), SortObjectData());

  // Hash object list
  for (const auto& o : gameState.objectData) {
    hash_combine(gameState.hash, o.name);

    // Hash the object variables
    for (const auto& value : o.variables) {
      hash_combine(gameState.hash, value);
    }
  }

  // Hash delayed actions
  for (const auto& d : gameState.delayedActionData) {
    hash_combine(gameState.hash, d.actionName);
    hash_combine(gameState.hash, d.orientationVector);
    hash_combine(gameState.hash, d.originatingPlayerId);
    hash_combine(gameState.hash, d.sourceObjectIdx);
    hash_combine(gameState.hash, d.priority);
  }
}

const GameState GameProcess::getGameState() {
  spdlog::debug("Getting game state");
  GameState gameState;

  const auto& stateMapping = getGameStateMapping();

  gameState.tickCount = *grid_->getTickCount();

  gameState.playerCount = grid_->getPlayerCount();
  gameState.grid.height = grid_->getHeight();
  gameState.grid.width = grid_->getWidth();

  const auto& globalVariables = grid_->getGlobalVariables();

  gameState.globalData.resize(globalVariables.size());

  // Serializing global variables
  for (const auto& globalVarIt : globalVariables) {
    auto variableName = globalVarIt.first;

    const auto variableNameIdx = stateMapping.globalVariableNameToIdx.at(variableName);
    auto variableValues = globalVarIt.second;

    gameState.globalData[variableNameIdx].resize(variableValues.size());

    for (const auto& variableValue : variableValues) {
      gameState.globalData[variableNameIdx][variableValue.first] = *variableValue.second;
    }
  }

  // Map of indexes to object pointers to use in delayed action serialization
  std::unordered_map<std::shared_ptr<Object>, uint32_t> objectPtrToIndex;
  // Serializing objects
  for (const auto& object : grid_->getObjects()) {
    uint32_t index = gameState.objectData.size();
    gameState.objectData.push_back(gdyFactory_->getObjectGenerator()->toObjectData(object));
    objectPtrToIndex.insert({object, index});
  }

  // Add default objects
  for(uint32_t p=0; p<grid_->getPlayerCount()+1; p++) {
    const auto& playerEmptyObject = grid_->getPlayerDefaultEmptyObject(p);
    gameState.objectData.push_back(gdyFactory_->getObjectGenerator()->toObjectData(playerEmptyObject));
    objectPtrToIndex.insert({playerEmptyObject, gameState.objectData.size()});

    const auto& playerBoundaryObject = grid_->getPlayerDefaultBoundaryObject(p);
    gameState.objectData.push_back(gdyFactory_->getObjectGenerator()->toObjectData(playerBoundaryObject));
    objectPtrToIndex.insert({playerBoundaryObject, gameState.objectData.size()});
  }

  // Serializing delayed actions
  auto delayedActions = grid_->getDelayedActions();
  for (const auto& delayedActionToCopy : delayedActions) {
    auto actionToCopy = delayedActionToCopy->action;
    auto playerId = delayedActionToCopy->playerId;
    auto sourceObjectMapping = actionToCopy->getSourceObject();
    const auto& clonedActionSourceObjectIt = objectPtrToIndex.find(sourceObjectMapping);

    if (clonedActionSourceObjectIt != objectPtrToIndex.end()) {
      DelayedActionData delayedActionData;
      auto remainingTicks = delayedActionToCopy->priority - gameState.tickCount;
      auto playerId = delayedActionToCopy->playerId;

      const auto& actionName = actionToCopy->getActionName();
      const auto& vectorToDest = actionToCopy->getVectorToDest();
      const auto& orientationVector = actionToCopy->getOrientationVector();
      const auto& originatingPlayerId = actionToCopy->getOriginatingPlayerId();

      delayedActionData.actionName = actionName;
      delayedActionData.playerId = playerId;
      delayedActionData.orientationVector = orientationVector;
      delayedActionData.originatingPlayerId = originatingPlayerId;
      delayedActionData.priority = remainingTicks;
      delayedActionData.vectorToDest = vectorToDest;
      delayedActionData.sourceObjectIdx = clonedActionSourceObjectIt->second;

      gameState.delayedActionData.push(delayedActionData);


    } else {
      spdlog::debug("Action serialization ignored as it is no longer valid.");
    }
  }

  generateStateHash(gameState);

  return gameState;
}

const GameStateMapping& GameProcess::getGameStateMapping() const {
  return gdyFactory_->getObjectGenerator()->getStateMapping();
}

}  // namespace griddly
