#include "TurnBasedGameProcess.hpp"

#include <spdlog/spdlog.h>

#include "DelayedActionQueueItem.hpp"
#include "Util/util.hpp"

namespace griddly {

const std::string TurnBasedGameProcess::name_ = "TurnBased";

TurnBasedGameProcess::TurnBasedGameProcess(
    ObserverType globalObserverType,
    std::shared_ptr<GDYFactory> gdyFactory,
    std::shared_ptr<Grid> grid)
    : GameProcess(globalObserverType, gdyFactory, grid) {
}

TurnBasedGameProcess::~TurnBasedGameProcess() {
}

ActionResult TurnBasedGameProcess::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
  spdlog::debug("Performing turn based actions for player {0}", playerId);

  if(requiresReset_) {
    throw std::runtime_error("Environment is in a terminated state and requires resetting.");
  }

  std::unordered_map<uint32_t, TerminationState> terminationState;
  int32_t reward = 0;

  auto stepRewards = grid_->performActions(playerId, actions);

  // rewards resulting from player actions
  accumulateRewards(accumulatedRewards_, stepRewards);

  if (updateTicks) {
    spdlog::debug("Updating Grid");
    auto delayedRewards = grid_->update();

    // rewards could come from delayed actions that are run at a particular time step
    accumulateRewards(accumulatedRewards_, delayedRewards);

    auto terminationResult = terminationHandler_->isTerminated();

    terminationState = terminationResult.playerStates;
    requiresReset_ = terminationResult.terminated;

    if (requiresReset_ && autoReset_) {
      reset();
    }
  }

  if (accumulatedRewards_[playerId] != 0) {
    reward = accumulatedRewards_[playerId];
    // reset reward for this player as they are being returned here
    accumulatedRewards_[playerId] = 0;
  }

  return {terminationState, requiresReset_, reward};
}

// This is only used in tests
void TurnBasedGameProcess::setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler) {
  terminationHandler_ = terminationHandler;
}

std::string TurnBasedGameProcess::getProcessName() const {
  return name_;
}

std::shared_ptr<TurnBasedGameProcess> TurnBasedGameProcess::clone() {
  // Firstly create a new grid
  std::shared_ptr<Grid> clonedGrid = std::shared_ptr<Grid>(new Grid());

  clonedGrid->setPlayerCount(players_.size());

  auto gridHeight = grid_->getHeight();
  auto gridWidth = grid_->getWidth();
  clonedGrid->resetMap(gridWidth, gridHeight);

  auto objectGenerator = gdyFactory_->getObjectGenerator();

  // Clone Global Variables
  spdlog::debug("Cloning global variables...");
  std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>> clonedGlobalVariables;
  for (auto globalVariableToCopy : grid_->getGlobalVariables()) {
    auto globalVariableName = globalVariableToCopy.first;
    auto playerVariableValues = globalVariableToCopy.second;

    for (auto playerVariable : playerVariableValues) {
      auto playerId = playerVariable.first;
      auto variableValue = *playerVariable.second;
      spdlog::debug("cloning {0}={1} for player {2}", globalVariableName, variableValue, playerId);
      clonedGlobalVariables[globalVariableName].insert({playerId, variableValue});
    }
  }
  clonedGrid->setGlobalVariables(clonedGlobalVariables);

  // Initialize Object Types
  spdlog::debug("Cloning objects types...");
  for (auto objectDefinition : objectGenerator->getObjectDefinitions()) {
    auto objectName = objectDefinition.second->objectName;
    std::vector<std::string> objectVariableNames;
    for (auto variableNameIt : objectDefinition.second->variableDefinitions) {
      objectVariableNames.push_back(variableNameIt.first);
    }
    clonedGrid->initObject(objectName, objectVariableNames);
  }

  // Clone Objects
  spdlog::debug("Cloning objects...");
  auto& objectsToCopy = grid_->getObjects();
  std::unordered_map<std::shared_ptr<Object>, std::shared_ptr<Object>> clonedObjectMapping;
  for (const auto& toCopy : objectsToCopy) {
    auto clonedObject = objectGenerator->cloneInstance(toCopy, clonedGrid->getGlobalVariables());
    clonedGrid->addObject(toCopy->getLocation(), clonedObject, false);

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
  for (auto delayedActionToCopy : delayedActions) {
    auto remainingTicks = delayedActionToCopy.priority - tickCountToCopy;
    auto actionToCopy = delayedActionToCopy.action;
    auto playerId = delayedActionToCopy.playerId;

    auto actionName = actionToCopy->getActionName();
    auto vectorToDest = actionToCopy->getVectorToDest();
    auto orientationVector = actionToCopy->getOrientationVector();
    auto sourceObjectMapping = actionToCopy->getSourceObject();

    auto clonedActionSourceObject = clonedObjectMapping[sourceObjectMapping];

    // Clone the action
    auto clonedAction = std::shared_ptr<Action>(new Action(clonedGrid, actionName, remainingTicks));

    // The orientation and vector to dest are already modified from the first action in respect
    // to if this is a relative action, so relative is set to false here
    clonedAction->init(clonedActionSourceObject, vectorToDest, orientationVector, false);

    clonedGrid->performActions(playerId, {clonedAction});
  }

  spdlog::debug("Cloning game process...");

  auto clonedGameProcess = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(globalObserverType_, gdyFactory_, clonedGrid));
  clonedGameProcess->setLevelGenerator(levelGenerator_);

  return clonedGameProcess;
}

}  // namespace griddly