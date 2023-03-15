#include "Grid.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "DelayedActionQueueItem.hpp"

namespace griddly {

Grid::Grid() : gameTicks_(std::make_shared<int32_t>(0)) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
  collisionDetectorFactory_ = std::make_shared<CollisionDetectorFactory>(CollisionDetectorFactory());
}

Grid::Grid(std::shared_ptr<CollisionDetectorFactory> collisionDetectorFactory) : gameTicks_(std::make_shared<int32_t>(0)) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  collisionDetectorFactory_ = std::move(collisionDetectorFactory);
}

Grid::~Grid() {
  spdlog::debug("Grid Destroyed");
  reset();
}

void Grid::setPlayerCount(uint32_t playerCount) {
  playerCount_ = playerCount;
}

uint32_t Grid::getPlayerCount() const {
  return playerCount_;
}

void Grid::resetMap(uint32_t width, uint32_t height) {
  spdlog::debug("Setting grid dimensions to: [{0}, {1}]", width, height);
  height_ = height;
  width_ = width;

  reset();

  globalVariables_["_steps"].insert({0, gameTicks_});

  if (updatedLocations_.empty()) {
    for (auto p = 0; p < playerCount_ + 1; p++) {
      updatedLocations_.emplace_back();
    }
  }
}

void Grid::reset() {
  occupiedLocations_.clear();
  objects_.clear();
  objectCounters_.clear();
  objectIds_.clear();
  objectVariableIds_.clear();
  delayedActions_ = {};
  defaultEmptyObject_.clear();
  defaultBoundaryObject_.clear();

  collisionObjectActionNames_.clear();
  collisionSourceObjectActionNames_.clear();
  collisionDetectors_.clear();
  collisionSourceObjects_.clear();

  *gameTicks_ = 0;
}

void Grid::setGlobalVariables(const std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>>& globalVariableDefinitions) {
  globalVariables_.clear();
  for (const auto& variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto playerVariables = variable.second;

    if (variableName == "_steps") {
      auto variableValue = playerVariables.at(0);
      *gameTicks_ = variableValue;
      globalVariables_["_steps"].insert({0, gameTicks_});
    } else {
      for (auto playerVariable : playerVariables) {
        auto playerId = playerVariable.first;
        auto variableValue = playerVariable.second;
        globalVariables_[variableName].insert({playerId, std::make_shared<int32_t>(variableValue)});
      }
    }

    for (auto playerVariable : playerVariables) {
      auto playerId = playerVariable.first;
      auto variableValue = playerVariable.second;
      globalVariables_[variableName].insert({playerId, std::make_shared<int32_t>(variableValue)});
    }
  }
}

void Grid::resetGlobalVariables(const std::map<std::string, GlobalVariableDefinition>& globalVariableDefinitions) {
  globalVariables_.clear();
  for (const auto& variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto variableDefinition = variable.second;

    if (variableDefinition.perPlayer) {
      for (int p = 0; p < playerCount_ + 1; p++) {
        globalVariables_[variableName].insert({p, std::make_shared<int32_t>(variableDefinition.initialValue)});
      }
    } else {
      globalVariables_[variableName].insert({0, std::make_shared<int32_t>(variableDefinition.initialValue)});
    }
  }
}

bool Grid::invalidateLocation(glm::ivec2 location) {
  for (int p = 0; p < playerCount_ + 1; p++) {
    updatedLocations_[p].insert(location);
  }
  return true;
}

void Grid::purgeUpdatedLocations(uint32_t player) {
  updatedLocations_[player].clear();
}

bool Grid::updateLocation(std::shared_ptr<Object> object, glm::ivec2 previousLocation, glm::ivec2 newLocation) {
  if (newLocation.x < 0 || newLocation.x >= width_ || newLocation.y < 0 || newLocation.y >= height_) {
    return false;
  }

  auto objectZIdx = object->getZIdx();
  auto newLocationObjects = occupiedLocations_[newLocation];

  if (newLocationObjects.find(objectZIdx) != newLocationObjects.end()) {
    spdlog::debug("Cannot move object {0} to location [{1}, {2}] as it is occupied.", object->getObjectName(), newLocation.x, newLocation.y);
    return false;
  }

  occupiedLocations_[previousLocation].erase(objectZIdx);
  occupiedLocations_[newLocation][objectZIdx] = object;

  invalidateLocation(previousLocation);
  invalidateLocation(newLocation);

  // Update spatial hashes if they exists
  if (!collisionDetectors_.empty()) {
    auto objectName = object->getObjectName();

    auto collisionDetectorActionNamesIt = collisionObjectActionNames_.find(object->getObjectName());
    if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
      auto collisionDetectorActionNames = collisionDetectorActionNamesIt->second;
      for (const auto& actionName : collisionDetectorActionNames) {
        auto collisionDetector = collisionDetectors_.at(actionName);
        spdlog::debug("Updating object {0} location in collision detector for action {1}", objectName, actionName);
        collisionDetector->upsert(object);
      }
    }
  }

  return true;
}

const std::unordered_set<glm::ivec2>& Grid::getUpdatedLocations(uint32_t playerId) const {
  if (playerId >= updatedLocations_.size()) {
    return EMPTY_LOCATIONS;
  }
  return updatedLocations_[playerId];
}

std::unordered_map<uint32_t, int32_t> Grid::executeAndRecord(uint32_t playerId, const std::shared_ptr<Action>& action) {
  if (recordEvents_) {
    auto event = buildGridEvent(action, playerId, *gameTicks_);
    auto reward = executeAction(playerId, action);
    recordGridEvent(event, reward);
    return reward;
  }
  return executeAction(playerId, action);
}

std::vector<uint32_t> Grid::filterBehaviourProbabilities(std::vector<uint32_t> actionBehaviourIdxs, std::vector<float> actionProbabilities) {
  std::vector<uint32_t> filteredBehaviours{};

  spdlog::debug("Action behaviour indexes to filter: {0}, probablilities to filter with: {1}", actionBehaviourIdxs.size(), actionProbabilities.size());

  for (uint32_t b = 0; b < actionBehaviourIdxs.size(); b++) {
    auto behaviourIdx = actionBehaviourIdxs[b];
    float executionProbability = actionProbabilities[behaviourIdx];
    spdlog::debug("Behaviour index: {0}, probability: {1}", behaviourIdx, executionProbability);
    if (executionProbability < 1.0) {
      auto actionProbability = randomGenerator_->sampleFloat(0, 1);
      if (actionProbability < executionProbability) {
        filteredBehaviours.push_back(behaviourIdx);
      }
    } else {
      filteredBehaviours.push_back(behaviourIdx);
    }
  }

  return filteredBehaviours;
}

std::unordered_map<uint32_t, int32_t> Grid::executeAction(uint32_t playerId, std::shared_ptr<Action> action) {
  auto sourceObject = action->getSourceObject();

  if (objects_.find(sourceObject) == objects_.end() && action->getDelay() > 0) {
    spdlog::debug("Delayed action for object that no longer exists.");
    return {};
  }

  auto destinationObject = action->getDestinationObject();

  // Need to get this name before anything happens to the object for example if the object is removed in onActionDst.
  auto destinationObjectName = destinationObject->getObjectName();

  if (sourceObject == nullptr) {
    spdlog::debug("Cannot perform action on empty space. ({0},{1})", action->getSourceLocation()[0], action->getSourceLocation()[1]);
    return {};
  }

  auto sourceObjectPlayerId = sourceObject->getPlayerId();

  if (playerId != 0 && sourceObjectPlayerId != playerId) {
    spdlog::debug("Cannot perform action on object not owned by player. Object owner {0}, Player owner {1}", sourceObjectPlayerId, playerId);
    return {};
  }

  if (playerId != 0 && sourceObject->isPlayerAvatar() && playerAvatars_.find(playerId) == playerAvatars_.end()) {
    spdlog::debug("Avatar for player {0} has been removed, action will be ignored.", playerId);
    return {};
  }

  auto validBehaviourIdxs = sourceObject->getValidBehaviourIdxs(action);

  auto filteredBehaviourIdxs = filterBehaviourProbabilities(validBehaviourIdxs, behaviourProbabilities_.at(action->getActionName()));

  if (filteredBehaviourIdxs.size() > 0) {
    std::unordered_map<uint32_t, int32_t> rewardAccumulator;
    auto dstBehaviourResult = destinationObject->onActionDst(action, validBehaviourIdxs);
    accumulateRewards(rewardAccumulator, dstBehaviourResult.rewards);

    if (dstBehaviourResult.abortAction) {
      spdlog::debug("Action {0} aborted by destination object behaviour.", action->getDescription());
      return rewardAccumulator;
    }

    auto srcBehaviourResult = sourceObject->onActionSrc(destinationObjectName, action, validBehaviourIdxs);
    accumulateRewards(rewardAccumulator, srcBehaviourResult.rewards);
    return rewardAccumulator;
  }
  spdlog::debug("Cannot perform action={0} on object={1}", action->getActionName(), sourceObject->getObjectName());
  return {};
}

GridEvent Grid::buildGridEvent(const std::shared_ptr<Action>& action, uint32_t playerId, uint32_t tick) const {
  auto sourceObject = action->getSourceObject();
  auto destObject = action->getDestinationObject();

  GridEvent event;
  event.playerId = playerId;
  event.actionName = action->getActionName();
  event.sourceObjectName = sourceObject->getObjectName();
  event.destObjectName = destObject->getObjectName();

  if (sourceObject->getObjectName() != "_empty") {
    event.sourceObjectPlayerId = sourceObject->getPlayerId();
  }

  if (destObject->getObjectName() != "_empty") {
    event.destinationObjectPlayerId = destObject->getPlayerId();
  }

  event.sourceLocation = action->getSourceLocation();
  event.destLocation = action->getDestinationLocation();

  event.tick = tick;
  event.delay = action->getDelay();

  return event;
}

void Grid::recordGridEvent(GridEvent event, std::unordered_map<uint32_t, int32_t> rewards) {
  event.rewards = std::move(rewards);
  eventHistory_.push_back(event);
}

std::unordered_map<uint32_t, int32_t> Grid::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  std::unordered_map<uint32_t, int32_t> rewardAccumulator;

  spdlog::trace("Tick {0}", *gameTicks_);

  // Have to add some non-determinism here for multi-agent games so player 1 doesn't always benefit
  std::shuffle(std::begin(actions), std::end(actions), randomGenerator_->getEngine());

  for (const auto& action : actions) {
    // Check if action is delayed or durative
    if (action->getDelay() > 0) {
      delayAction(playerId, action);
    } else {
      auto actionRewards = executeAndRecord(playerId, action);
      accumulateRewards(rewardAccumulator, actionRewards);
    }
  }

  return rewardAccumulator;
}

void Grid::delayAction(uint32_t playerId, std::shared_ptr<Action> action) {
  auto executionTarget = *(gameTicks_) + action->getDelay();
  spdlog::debug("Delaying action={0} to execution target time {1}", action->getDescription(), executionTarget);
  delayedActions_.push(std::make_shared<DelayedActionQueueItem>(DelayedActionQueueItem{playerId, executionTarget, action}));
}

std::unordered_map<uint32_t, int32_t> Grid::processDelayedActions() {
  std::unordered_map<uint32_t, int32_t> delayedRewards;

  spdlog::debug("{0} Delayed actions at game tick {1}", delayedActions_.size(), *gameTicks_);

  // Perform any delayed actions
  std::vector<std::shared_ptr<DelayedActionQueueItem>> actionsToExecute;
  while (!delayedActions_.empty() && delayedActions_.top()->priority <= *(gameTicks_)) {
    // Get the top element and remove it
    actionsToExecute.push_back(delayedActions_.top());
    delayedActions_.pop();
  }

  for (const auto& delayedAction : actionsToExecute) {
    auto action = delayedAction->action;
    auto playerId = delayedAction->playerId;

    spdlog::debug("Popped delayed action {0} at game tick {1}", action->getDescription(), *gameTicks_);

    auto delayedActionRewards = executeAndRecord(playerId, action);
    accumulateRewards(delayedRewards, delayedActionRewards);
  }

  return delayedRewards;
}

std::unordered_map<uint32_t, int32_t> Grid::processCollisions() {
  std::unordered_map<uint32_t, int32_t> collisionRewards;

  if (collisionDetectors_.empty()) {
    return collisionRewards;
  }

  // Check for collisions
  for (const auto& object : collisionSourceObjects_) {
    const auto& objectName = object->getObjectName();
    auto collisionActionNamesIt = collisionSourceObjectActionNames_.find(objectName);
    if (collisionActionNamesIt != collisionSourceObjectActionNames_.end()) {
      const auto& collisionActionNames = collisionSourceObjectActionNames_.at(objectName);
      auto location = object->getLocation();
      auto playerId = object->getPlayerId();

      for (const auto& actionName : collisionActionNames) {
        spdlog::debug("Collision detector under action {0} for object {1} being queried", actionName, objectName);
        auto collisionDetector = collisionDetectors_.at(actionName);
        auto& actionTriggerDefinition = actionTriggerDefinitions_.at(actionName);

        auto searchLocationOffset = actionTriggerDefinition.offset;
        if (actionTriggerDefinition.relative) {
          searchLocationOffset = searchLocationOffset * object->getObjectOrientation().getRotationMatrix();
        }

        const auto collisionLocation = location + searchLocationOffset;

        auto searchResults = collisionDetector->search(collisionLocation);

        auto objectsInCollisionRange = searchResults.objectSet;

        for (const auto& collisionObject : objectsInCollisionRange) {
          if (collisionObject == object) {
            {
              continue;
            }
          }

          spdlog::debug("Collision detected for action {0} {1}->{2}", actionName, collisionObject->getObjectName(), objectName);

          std::shared_ptr<Action> collisionAction = std::make_shared<Action>(Action(shared_from_this(), actionName, playerId, 0));
          collisionAction->init(object, collisionObject);

          auto rewards = executeAndRecord(0, collisionAction);

          accumulateRewards(collisionRewards, rewards);
        }
      }
    }
  }

  return collisionRewards;
}

std::unordered_map<uint32_t, int32_t> Grid::update() {
  *(gameTicks_) += 1;

  std::unordered_map<uint32_t, int32_t> rewards;

  auto delayedActionRewards = processDelayedActions();
  spdlog::debug("Delayed actions processed");

  accumulateRewards(rewards, delayedActionRewards);

  auto collisionRewards = processCollisions();
  spdlog::debug("Processed collisions");
  accumulateRewards(rewards, collisionRewards);

  return rewards;
}

const DelayedActionQueue& Grid::getDelayedActions() {
  return delayedActions_;
}

std::shared_ptr<int32_t> Grid::getTickCount() const {
  return gameTicks_;
}

void Grid::setTickCount(int32_t tickCount) {
  *gameTicks_ = tickCount;
}

const std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

const TileObjects& Grid::getObjectsAt(glm::ivec2 location) const {
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return EMPTY_OBJECTS;
  }
  return i->second;
}

std::shared_ptr<Object> Grid::getObject(glm::ivec2 location) const {
  if (occupiedLocations_.count(location) > 0) {
    const auto& objectsAtLocation = occupiedLocations_.at(location);
    if (!objectsAtLocation.empty()) {
      // Get the highest index object
      return objectsAtLocation.rbegin()->second;
    }
  }

  return nullptr;
}

const std::unordered_map<std::string, uint32_t>& Grid::getObjectIds() const {
  return objectIds_;
}

const std::unordered_map<std::string, uint32_t>& Grid::getObjectVariableIds() const {
  return objectVariableIds_;
}

const std::vector<std::string> Grid::getObjectNames() const {
  auto namesCount = objectIds_.size();
  std::vector<std::string> orderedNames(namesCount);

  for (const auto& objectIdIt : objectIds_) {
    auto name = objectIdIt.first;
    auto idx = objectIdIt.second;
    orderedNames[idx] = name;
  }

  return orderedNames;
}

const std::vector<std::string> Grid::getAllObjectVariableNames() const {
  auto namesCount = objectVariableIds_.size();
  std::vector<std::string> orderedNames(namesCount);

  for (const auto& objectVariableIdIt : objectVariableIds_) {
    auto name = objectVariableIdIt.first;
    auto idx = objectVariableIdIt.second;
    orderedNames[idx] = name;
  }

  return orderedNames;
}

const std::unordered_map<std::string, std::vector<std::string>> Grid::getObjectVariableMap() const {
  return objectVariableMap_;
}

void Grid::initObject(std::string objectName, std::vector<std::string> variableNames) {
  objectIds_.insert({objectName, objectIds_.size()});

  objectCounters_.insert({objectName, {{0, std::make_shared<int32_t>(0)}}});

  for (auto& variableName : variableNames) {
    objectVariableIds_.insert({variableName, objectVariableIds_.size()});
  }

  objectVariableMap_[objectName] = variableNames;
}

std::unordered_map<uint32_t, std::shared_ptr<int32_t>> Grid::getObjectCounter(std::string objectName) {
  auto objectCounterIt = objectCounters_.find(objectName);
  if (objectCounterIt == objectCounters_.end()) {
    objectCounters_[objectName][0] = std::make_shared<int32_t>(0);
    return objectCounters_.at(objectName);
  }

  return objectCounterIt->second;
}

const std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>& Grid::getGlobalVariables() const {
  return globalVariables_;
}

void Grid::setBehaviourProbabilities(const std::unordered_map<std::string, std::vector<float>>& behaviourProbabilities) {
  behaviourProbabilities_ = behaviourProbabilities;
}

void Grid::addCollisionDetector(std::unordered_set<std::string> objectNames, std::string actionName, std::shared_ptr<CollisionDetector> collisionDetector) {
  for (const auto& objectName : objectNames) {
    collisionObjectActionNames_[objectName].insert(actionName);

    spdlog::debug("Adding collision detector with name {0} for action {1}", objectName, actionName);
  }

  collisionDetectors_.insert({actionName, collisionDetector});

  // If we are adding an collision detector, make sure that all required objects are added to it.
  for (const auto& object : objects_) {
    const auto& objectName = object->getObjectName();
    if (objectNames.find(objectName) != objectNames.end()) {
      collisionDetector->upsert(object);
    }
  }
}

void Grid::addActionTrigger(std::string actionName, ActionTriggerDefinition actionTriggerDefinition) {
  std::shared_ptr<CollisionDetector> collisionDetector = collisionDetectorFactory_->newCollisionDetector(width_, height_, actionTriggerDefinition);

  std::unordered_set<std::string> objectNames;
  for (const auto& sourceObjectName : actionTriggerDefinition.sourceObjectNames) {
    // TODO: I dont think we need to add source names to all object names?
    // objectNames.push_back(sourceObjectName);
    collisionSourceObjectActionNames_[sourceObjectName].insert(actionName);
  }

  for (const auto& destinationObjectName : actionTriggerDefinition.destinationObjectNames) {
    objectNames.insert(destinationObjectName);
    collisionObjectActionNames_[destinationObjectName].insert(actionName);
  }

  actionTriggerDefinitions_.insert({actionName, actionTriggerDefinition});

  addCollisionDetector(objectNames, actionName, collisionDetector);
}

void Grid::addPlayerDefaultEmptyObject(std::shared_ptr<Object> emptyObject) {
  spdlog::debug("Adding default empty object for player {0}", emptyObject->getPlayerId());
  emptyObject->init({-1, -1});
  defaultEmptyObject_[emptyObject->getPlayerId()] = emptyObject;
}


void Grid::addPlayerDefaultBoundaryObject(std::shared_ptr<Object> boundaryObject) {
  spdlog::debug("Adding default boundary object for player {0}", boundaryObject->getPlayerId());
  boundaryObject->init({-1, -1});
  defaultBoundaryObject_[boundaryObject->getPlayerId()] = boundaryObject;
}

std::shared_ptr<Object> Grid::getPlayerDefaultEmptyObject(uint32_t playerId) const {
  spdlog::debug("Getting default empty object for player {0}", playerId);
  return defaultEmptyObject_.at(playerId);
}

std::shared_ptr<Object> Grid::getPlayerDefaultBoundaryObject(uint32_t playerId) const {
  spdlog::debug("Getting default boundary object for player {0}", playerId);
  return defaultBoundaryObject_.at(playerId);
}

void Grid::addObject(glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions, std::shared_ptr<Action> originatingAction, DiscreteOrientation orientation) {
  const auto& objectName = object->getObjectName();
  auto playerId = object->getPlayerId();

  if (playerId > getPlayerCount()) {
    throwRuntimeError(fmt::format("Cannot add object {0} with player id {1} as the environment is only configured for {2} players.", objectName, playerId, playerCount_));
  }

  if (object->isPlayerAvatar()) {
    // If there is no playerId set on the object, we should set the playerId to 1 as 0 is reserved
    spdlog::debug("Player avatar (playerId:{3}) set as object={0} at location [{1}, {2}]", object->getObjectName(), location.x, location.y, playerId);
    playerAvatars_[playerId] = object;
  }

  spdlog::debug("Adding object={0} belonging to player {1} to location: [{2},{3}]", objectName, playerId, location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->init(location, orientation);

    auto objectZIdx = object->getZIdx();
    auto& objectsAtLocation = occupiedLocations_[location];

    auto objectAtZIt = objectsAtLocation.find(objectZIdx);

    // If we find an in this location with the same zindex, do not add it.
    if (objectAtZIt != objectsAtLocation.end()) {
      spdlog::error("Cannot add object={0} to location: [{1},{2}], there is already an object here.", objectName, location.x, location.y);
      objects_.erase(object);
    } else {
      auto objectCountersForPlayers = objectCounters_[objectName];

      // Initialize the counter if it does not exist
      auto objectCounterForPlayerIt = objectCountersForPlayers.find(playerId);
      if (objectCounterForPlayerIt == objectCountersForPlayers.end()) {
        objectCounters_[objectName][playerId] = std::make_shared<int32_t>(0);
      }

      *objectCounters_[objectName][playerId] += 1;
      objectsAtLocation.insert({objectZIdx, object});
      invalidateLocation(location);
    }

    if (applyInitialActions) {
      auto initialActions = object->getInitialActions(std::move(originatingAction));
      if (!initialActions.empty()) {
        spdlog::debug("Performing {0} Initial actions on object {1}.", initialActions.size(), objectName);
        performActions(0, initialActions);
      }
    }

    if (!collisionDetectors_.empty()) {
      const auto& collisionDetectorActionNamesIt = collisionObjectActionNames_.find(objectName);
      if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
        const auto& collisionDetectorActionNames = collisionObjectActionNames_.at(objectName);
        for (const auto& actionName : collisionDetectorActionNames) {
          auto collisionDetector = collisionDetectors_.at(actionName);
          spdlog::debug("Adding object {0} to collision detector for action {1}", objectName, actionName);
          collisionDetector->upsert(object);
        }
      }

      auto collisionActionNamesIt = collisionSourceObjectActionNames_.find(objectName);
      if (collisionActionNamesIt != collisionSourceObjectActionNames_.end()) {
        collisionSourceObjects_.insert(object);
      }
    }

  } else {
    spdlog::error("Cannot add object={0} to location: [{1},{2}]", objectName, location.x, location.y);
  }
}

void Grid::seedRandomGenerator(uint32_t seed) {
  randomGenerator_->seed(seed);
}

std::shared_ptr<RandomGenerator> Grid::getRandomGenerator() const {
  return randomGenerator_;
}

bool Grid::removeObject(std::shared_ptr<Object> object) {
  const auto& objectName = object->getObjectName();
  auto playerId = object->getPlayerId();
  auto location = object->getLocation();
  auto objectZIdx = object->getZIdx();
  spdlog::debug("Removing object={0} with playerId={1} from environment.", object->getDescription(), playerId);

  if (objects_.erase(object) > 0 && occupiedLocations_[location].erase(objectZIdx) > 0) {
    *objectCounters_[objectName][playerId] -= 1;
    invalidateLocation(location);

    // if we are removing a player's avatar
    if (!playerAvatars_.empty() && playerId != 0) {
      auto playerAvatarIt = playerAvatars_.find(playerId);
      if (playerAvatarIt != playerAvatars_.end() && playerAvatarIt->second == object) {
        spdlog::debug("Removing player {0} avatar {1}", playerId, objectName);
        playerAvatars_.erase(playerId);
      }
    }

    if (!collisionDetectors_.empty()) {
      auto collisionDetectorActionNamesIt = collisionObjectActionNames_.find(objectName);
      if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
        auto collisionDetectorActionNames = collisionDetectorActionNamesIt->second;
        for (const auto& actionName : collisionDetectorActionNames) {
          auto collisionDetector = collisionDetectors_.at(actionName);
          collisionDetector->remove(object);
        }
      }

      collisionSourceObjects_.erase(object);
    }

    return true;
  }

  spdlog::error("Could not remove object={0} from environment.", object->getDescription());
  return false;
}

std::unordered_map<uint32_t, std::shared_ptr<Object>> Grid::getPlayerAvatarObjects() const {
  return playerAvatars_;
}

uint32_t Grid::getWidth() const { return width_; }

uint32_t Grid::getHeight() const { return height_; }

void Grid::enableHistory(bool enable) {
  recordEvents_ = enable;
}

const std::vector<GridEvent>& Grid::getHistory() const {
  return eventHistory_;
}

void Grid::purgeHistory() {
  eventHistory_.clear();
}

const std::unordered_map<std::string, std::shared_ptr<CollisionDetector>>& Grid::getCollisionDetectors() const {
  return collisionDetectors_;
}

const std::unordered_map<std::string, ActionTriggerDefinition>& Grid::getActionTriggerDefinitions() const {
  return actionTriggerDefinitions_;
}

const std::unordered_map<std::string, std::unordered_set<std::string>>& Grid::getSourceObjectCollisionActionNames() const {
  return collisionSourceObjectActionNames_;
}

const std::unordered_map<std::string, std::unordered_set<std::string>>& Grid::getObjectCollisionActionNames() const {
  return collisionObjectActionNames_;
}

}  // namespace griddly