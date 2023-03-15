#include "TurnBasedGameProcess.hpp"

#include <spdlog/spdlog.h>

#include <utility>

#include "DelayedActionQueueItem.hpp"
#include "Util/util.hpp"

namespace griddly {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(
    std::string globalObserverName,
    std::shared_ptr<GDYFactory> gdyFactory,
    std::shared_ptr<Grid> grid)
    : GameProcess(globalObserverName, std::move(gdyFactory), std::move(grid)) {
}

TurnBasedGameProcess::~TurnBasedGameProcess() {
  spdlog::debug("TurnBasedGameProcess Destroyed");
}

ActionResult TurnBasedGameProcess::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
  spdlog::debug("Performing turn based actions for player {0}", playerId);

  if (requiresReset_) {
    auto error = fmt::format("Environment is in a terminated state and requires resetting.");
    spdlog::error(error);
    throw std::runtime_error(error);
  }

  std::unordered_map<uint32_t, TerminationState> terminationState;
  auto stepRewards = grid_->performActions(playerId, actions);

  // rewards resulting from player actions
  for (auto valueIt : stepRewards) {
    spdlog::debug("Accumulating step reward for player {0}. {1} += {2}", valueIt.first, accumulatedRewards_[valueIt.first], valueIt.second);
  }
  accumulateRewards(accumulatedRewards_, stepRewards);

  if (updateTicks) {
    spdlog::debug("Updating Grid");
    auto delayedRewards = grid_->update();

    // rewards could come from delayed actions that are run at a particular time step
    for (auto valueIt : delayedRewards) {
      spdlog::debug("Accumulating delayed reward for player {0}. {1} += {2}", valueIt.first, accumulatedRewards_[valueIt.first], valueIt.second);
    }
    accumulateRewards(accumulatedRewards_, delayedRewards);

    auto terminationResult = terminationHandler_->isTerminated();

    terminationState = terminationResult.playerStates;
    requiresReset_ = terminationResult.terminated;

    for (auto valueIt : terminationResult.rewards) {
      spdlog::debug("Accumulating termination reward for player {0}. {1} += {2}", valueIt.first, accumulatedRewards_[valueIt.first], valueIt.second);
    }
    accumulateRewards(accumulatedRewards_, terminationResult.rewards);

    if (requiresReset_ && autoReset_) {
      reset();
    }
  }

  return {terminationState, requiresReset_};
}

// This is only used in tests
void TurnBasedGameProcess::setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler) {
  terminationHandler_ = std::move(terminationHandler);
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

std::shared_ptr<TurnBasedGameProcess> TurnBasedGameProcess::fromGameState(GameState& gameState) {
  std::shared_ptr<Grid> loadedGrid = std::make_shared<Grid>(Grid());

  loadedGrid->setPlayerCount(gameState.playerCount);

  auto gridHeight = gameState.grid.height;
  auto gridWidth = gameState.grid.width;
  loadedGrid->resetMap(gridWidth, gridHeight);

  auto objectGenerator = gdyFactory_->getObjectGenerator();
  const auto& stateMapping = objectGenerator->getStateMapping();

  // Clone Global Variables
  spdlog::debug("Loading global variables...");
  std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>> loadedGlobalVariables;
  for (const auto& globalVariableIdx : stateMapping.globalVariableNameToIdx) {
    auto globalVariableName = globalVariableIdx.first;
    auto playerVariableValues = gameState.globalData[globalVariableIdx.second];

    for (uint32_t playerId = 0; playerId < playerVariableValues.size(); playerId++) {
      const auto& variableValue = playerVariableValues[playerId];
      spdlog::debug("Loading {0}={1} for player {2}", globalVariableName, variableValue, playerId);
      loadedGlobalVariables[globalVariableName].insert({playerId, variableValue});
    }
  }
  loadedGrid->setGlobalVariables(loadedGlobalVariables);

  // Initialize Object Types
  spdlog::debug("Loading objects types...");
  for (const auto& objectDefinition : objectGenerator->getObjectDefinitions()) {
    const auto& objectName = objectDefinition.second->objectName;

    // do not initialize these objects
    if (objectName == "_empty" || objectName == "_boundary") {
      continue;
    }
    std::vector<std::string> objectVariableNames;
    for (const auto& variableNameIt : objectDefinition.second->variableDefinitions) {
      objectVariableNames.push_back(variableNameIt.first);
    }
    loadedGrid->initObject(objectName, objectVariableNames);

    spdlog::debug("Loaded object type {0} with {1} variables", objectName, objectVariableNames.size());
  }

  std::unordered_map<uint32_t, std::shared_ptr<Object>> loadedObjectMapping;

  // Behaviour probabilities
  loadedGrid->setBehaviourProbabilities(objectGenerator->getBehaviourProbabilities());

  // Clone Objects
  spdlog::debug("Loading {0} objects...", gameState.objectData.size());
  const auto& objectsToLoad = gameState.objectData;
  for (uint32_t loadIdx = 0; loadIdx < objectsToLoad.size(); loadIdx++) {
    const auto& toLoad = objectsToLoad[loadIdx];

    spdlog::debug("Loading object: {0} with {1} variables", toLoad.name, toLoad.variables.size());

    const auto& variableIndexes = toLoad.getVariableIndexes(stateMapping);
    auto loadedObject = objectGenerator->fromObjectData(toLoad, loadedGrid);
    if (loadedObject->getObjectName() == "_empty") {
      loadedGrid->addPlayerDefaultEmptyObject(loadedObject);
    } else if (loadedObject->getObjectName() == "_boundary") {
      loadedGrid->addPlayerDefaultBoundaryObject(loadedObject);
    } else {
      loadedGrid->addObject(toLoad.getLocation(variableIndexes), loadedObject, false, nullptr, toLoad.getOrientation(variableIndexes));
    }
    // We need to know which objects are equivalent in the grid so we can
    // map delayed actions later
    loadedObjectMapping[loadIdx] = loadedObject;
  }

  // Copy Game Timer
  spdlog::debug("Loading game timer state...");
  auto tickCountToCopy = gameState.tickCount;
  loadedGrid->setTickCount(tickCountToCopy);

  // Load Delayed actions
  spdlog::debug("Loading delayed actions...");
  for (const auto& delayedActionToLoad : gameState.delayedActionData) {
    const auto remainingTicks = delayedActionToLoad.priority - tickCountToCopy;
    const auto& actionName = delayedActionToLoad.actionName;
    const auto playerId = delayedActionToLoad.playerId;
    const auto originatingPlayerId = delayedActionToLoad.originatingPlayerId;
    const auto& vectorToDest = delayedActionToLoad.vectorToDest;
    const auto& orientationVector = delayedActionToLoad.orientationVector;

    const auto sourceObjectIdx = delayedActionToLoad.sourceObjectIdx;

    spdlog::debug("Loading action {0}", actionName);

    auto loadedActionSourceObjectIt = loadedObjectMapping.find(sourceObjectIdx);

    if (loadedActionSourceObjectIt != loadedObjectMapping.end()) {
      auto loadedAction = std::make_shared<Action>(Action(loadedGrid, actionName, originatingPlayerId, remainingTicks));

      // The orientation and vector to dest are already modified from the first action in respect
      // to if this is a relative action, so relative is set to false here
      loadedAction->init(loadedActionSourceObjectIt->second, vectorToDest, orientationVector, false);

      spdlog::debug("Applying loaded action {0}", loadedAction->getActionName());
      loadedGrid->performActions(playerId, {loadedAction});
    } else {
      spdlog::debug("Action cannot be loaded as it is invalid in original environment.");
    }
  }

  spdlog::debug("Loading game process...");

  auto clonedGameProcess = std::make_shared<TurnBasedGameProcess>(TurnBasedGameProcess(globalObserverName_, gdyFactory_, loadedGrid));
  clonedGameProcess->setLevelGenerator(levelGenerator_);

  return clonedGameProcess;
}

std::shared_ptr<TurnBasedGameProcess> TurnBasedGameProcess::clone() {
  // Firstly create a new grid
  std::shared_ptr<Grid> clonedGrid = std::make_shared<Grid>(Grid());

  clonedGrid->setPlayerCount(grid_->getPlayerCount());

  auto gridHeight = grid_->getHeight();
  auto gridWidth = grid_->getWidth();
  clonedGrid->resetMap(gridWidth, gridHeight);

  auto objectGenerator = gdyFactory_->getObjectGenerator();

  // Clone Global Variables
  spdlog::debug("Cloning global variables...");
  std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>> clonedGlobalVariables;
  for (const auto& globalVariableToCopy : grid_->getGlobalVariables()) {
    auto globalVariableName = globalVariableToCopy.first;
    auto playerVariableValues = globalVariableToCopy.second;

    for (const auto& playerVariable : playerVariableValues) {
      auto playerId = playerVariable.first;
      auto variableValue = *playerVariable.second;
      spdlog::debug("Cloning {0}={1} for player {2}", globalVariableName, variableValue, playerId);
      clonedGlobalVariables[globalVariableName].insert({playerId, variableValue});
    }
  }
  clonedGrid->setGlobalVariables(clonedGlobalVariables);

  // Initialize Object Types
  spdlog::debug("Cloning objects types...");
  for (const auto& objectDefinition : objectGenerator->getObjectDefinitions()) {
    auto objectName = objectDefinition.second->objectName;

    // do not initialize these objects
    if (objectName == "_empty" || objectName == "_boundary") {
      continue;
    }
    std::vector<std::string> objectVariableNames;
    for (const auto& variableNameIt : objectDefinition.second->variableDefinitions) {
      objectVariableNames.push_back(variableNameIt.first);
    }
    clonedGrid->initObject(objectName, objectVariableNames);
  }

  std::unordered_map<std::shared_ptr<Object>, std::shared_ptr<Object>> clonedObjectMapping;

  // Adding player default objects
  for (auto playerId = 0; playerId < players_.size() + 1; playerId++) {
    auto defaultEmptyObject = objectGenerator->newInstance("_empty", playerId, clonedGrid);
    auto defaultBoundaryObject = objectGenerator->newInstance("_boundary", playerId, clonedGrid);
    clonedGrid->addPlayerDefaultBoundaryObject(defaultBoundaryObject);
    clonedGrid->addPlayerDefaultEmptyObject(defaultEmptyObject);

    auto defaultEmptyObjectToCopy = grid_->getPlayerDefaultEmptyObject(playerId);
    auto defaultBoundaryObjectToCopy = grid_->getPlayerDefaultBoundaryObject(playerId);

    clonedObjectMapping[defaultEmptyObjectToCopy] = defaultEmptyObject;
    clonedObjectMapping[defaultBoundaryObjectToCopy] = defaultBoundaryObject;
  }

  // Behaviour probabilities
  clonedGrid->setBehaviourProbabilities(objectGenerator->getBehaviourProbabilities());

  // Clone Objects
  spdlog::debug("Cloning objects...");
  const auto& objectsToCopy = grid_->getObjects();
  for (const auto& toCopy : objectsToCopy) {
    auto clonedObject = objectGenerator->cloneInstance(toCopy, clonedGrid);
    clonedGrid->addObject(toCopy->getLocation(), clonedObject, false, nullptr, toCopy->getObjectOrientation());
    // We need to know which objects are equivalent in the grid so we can
    // map delayed actions later
    clonedObjectMapping[toCopy] = clonedObject;
  }

  // Copy Game Timer
  spdlog::debug("Cloning game timer state...");
  auto tickCountToCopy = *grid_->getTickCount();
  clonedGrid->setTickCount(tickCountToCopy);

  // Clone Delayed actions
  auto delayedActions = grid_->getDelayedActions();

  spdlog::debug("Cloning delayed actions...");
  for (const auto& delayedActionToCopy : delayedActions) {
    auto remainingTicks = delayedActionToCopy->priority - tickCountToCopy;
    auto actionToCopy = delayedActionToCopy->action;
    auto playerId = delayedActionToCopy->playerId;

    auto actionName = actionToCopy->getActionName();
    auto vectorToDest = actionToCopy->getVectorToDest();
    auto orientationVector = actionToCopy->getOrientationVector();
    auto sourceObjectMapping = actionToCopy->getSourceObject();
    auto originatingPlayerId = actionToCopy->getOriginatingPlayerId();
    spdlog::debug("Copying action {0}", actionToCopy->getActionName());

    auto clonedActionSourceObjectIt = clonedObjectMapping.find(sourceObjectMapping);

    if (clonedActionSourceObjectIt != clonedObjectMapping.end()) {
      // Clone the action
      auto clonedAction = std::make_shared<Action>(Action(clonedGrid, actionName, originatingPlayerId, remainingTicks));

      // The orientation and vector to dest are already modified from the first action in respect
      // to if this is a relative action, so relative is set to false here
      clonedAction->init(clonedActionSourceObjectIt->second, vectorToDest, orientationVector, false);

      spdlog::debug("applying cloned action {0}", clonedAction->getActionName());
      clonedGrid->performActions(playerId, {clonedAction});
    } else {
      spdlog::debug("Action cannot be cloned as it is invalid in original environment.");
    }
  }

  spdlog::debug("Cloning game process...");

  auto clonedGameProcess = std::make_shared<TurnBasedGameProcess>(TurnBasedGameProcess(globalObserverName_, gdyFactory_, clonedGrid));
  clonedGameProcess->setLevelGenerator(levelGenerator_);

  return clonedGameProcess;
}

void TurnBasedGameProcess::seedRandomGenerator(uint32_t seed) {
  grid_->seedRandomGenerator(seed);
}

}  // namespace griddly