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

Grid::Grid() {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
  collisionDetectorFactory_ = std::shared_ptr<CollisionDetectorFactory>(new CollisionDetectorFactory());
}

Grid::Grid(std::shared_ptr<CollisionDetectorFactory> collisionDetectorFactory) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  collisionDetectorFactory_ = collisionDetectorFactory;
}

void Grid::setPlayerCount(uint32_t playerCount) {
  playerCount_ = playerCount;
}

void Grid::resetMap(uint32_t width, uint32_t height) {
  spdlog::debug("Setting grid dimensions to: [{0}, {1}]", width, height);
  height_ = height;
  width_ = width;

  occupiedLocations_.clear();
  objects_.clear();
  objectCounters_.clear();
  objectIds_.clear();
  objectVariableIds_.clear();
  delayedActions_ = {};
  defaultObject_ = {};

  collisionObjectActionNames_.clear();
  collisionSourceObjectActionNames_.clear();
  collisionDetectors_.clear();

  gameTicks_ = std::make_shared<int32_t>(0);

  if (updatedLocations_.size() == 0) {
    for (auto p = 0; p < playerCount_ + 1; p++) {
      updatedLocations_.push_back(std::unordered_set<glm::ivec2>{});
    }
  }
}

void Grid::setGlobalVariables(std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>> globalVariableDefinitions) {
  globalVariables_.clear();
  for (auto variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto playerVariables = variable.second;

    for (auto playerVariable : playerVariables) {
      auto playerId = playerVariable.first;
      auto variableValue = playerVariable.second;
      globalVariables_[variableName].insert({playerId, std::make_shared<int32_t>(variableValue)});
    }
  }
}

void Grid::resetGlobalVariables(std::unordered_map<std::string, GlobalVariableDefinition> globalVariableDefinitions) {
  globalVariables_.clear();
  for (auto variable : globalVariableDefinitions) {
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
  if (collisionDetectors_.size() > 0) {
    auto objectName = object->getObjectName();

    auto collisionDetectorActionNamesIt = collisionObjectActionNames_.find(object->getObjectName());
    if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
      auto collisionDetectorActionNames = collisionDetectorActionNamesIt->second;
      for (const auto& actionName : collisionDetectorActionNames) {
        auto collisionDetector = collisionDetectors_.at(actionName);
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

std::unordered_map<uint32_t, int32_t> Grid::executeAndRecord(uint32_t playerId, std::shared_ptr<Action> action) {
  if (recordEvents_) {
    auto event = buildGridEvent(action, playerId, *gameTicks_);
    auto reward = executeAction(playerId, action);
    recordGridEvent(event, reward);
    return reward;
  } else {
    return executeAction(playerId, action);
  }
}

std::unordered_map<uint32_t, int32_t> Grid::executeAction(uint32_t playerId, std::shared_ptr<Action> action) {
  

  float executionProbability = 1.0;

  auto executionProbabilityIt = actionProbabilities_.find(action->getActionName());
  if(executionProbabilityIt != actionProbabilities_.end()) {
    executionProbability = executionProbabilityIt->second;
  }

  spdlog::debug("Executing action {0} with probability {1}", action->getDescription(), executionProbability);

  if (executionProbability < 1.0) {
    // TODO: Can this be cleaned up a bit maybe static variables or someting?
    std::random_device rd;
    std::mt19937 randomGenerator(rd());
    std::uniform_real_distribution<float> actionExecutionDistribution;
    auto actionProbability = actionExecutionDistribution(randomGenerator);
    if (actionProbability > executionProbability) {
      spdlog::debug("Action aborted due to probability check {0} > {1}", actionProbability, executionProbability);
      return {};
    }
  }

  auto sourceObject = action->getSourceObject();
  auto destinationObject = action->getDestinationObject();

  // Need to get this name before anything happens to the object for example if the object is removed in onActionDst.
  auto originalDestinationObjectName = destinationObject == nullptr ? "_empty" : destinationObject->getObjectName();

  if (objects_.find(sourceObject) == objects_.end() && action->getDelay() > 0) {
    spdlog::debug("Delayed action for object that no longer exists.");
    return {};
  }

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

  if (sourceObject->isValidAction(action)) {
    std::unordered_map<uint32_t, int32_t> rewardAccumulator;
    if (destinationObject != nullptr && destinationObject.get() != sourceObject.get()) {
      auto dstBehaviourResult = destinationObject->onActionDst(action);
      accumulateRewards(rewardAccumulator, dstBehaviourResult.rewards);

      if (dstBehaviourResult.abortAction) {
        spdlog::debug("Action {0} aborted by destination object behaviour.", action->getDescription());
        return rewardAccumulator;
      }
    }

    auto srcBehaviourResult = sourceObject->onActionSrc(originalDestinationObjectName, action);
    accumulateRewards(rewardAccumulator, srcBehaviourResult.rewards);
    return rewardAccumulator;

  } else {
    spdlog::debug("Cannot perform action={0} on object={1}", action->getActionName(), sourceObject->getObjectName());
    return {};
  }
}

GridEvent Grid::buildGridEvent(std::shared_ptr<Action> action, uint32_t playerId, uint32_t tick) {
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
  event.rewards = rewards;
  eventHistory_.push_back(event);
}

std::unordered_map<uint32_t, int32_t> Grid::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  std::unordered_map<uint32_t, int32_t> rewardAccumulator;

  spdlog::trace("Tick {0}", *gameTicks_);

  for (auto action : actions) {
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
  delayedActions_.push(DelayedActionQueueItem{playerId, executionTarget, action});
}

std::unordered_map<uint32_t, int32_t> Grid::processDelayedActions() {
  std::unordered_map<uint32_t, int32_t> delayedRewards;

  spdlog::debug("{0} Delayed actions at game tick {1}", delayedActions_.size(), *gameTicks_);
  // Perform any delayed actions

  std::vector<DelayedActionQueueItem> actionsToExecute;
  while (delayedActions_.size() > 0 && delayedActions_.top().priority <= *(gameTicks_)) {
    // Get the top element and remove it
    actionsToExecute.push_back(delayedActions_.top());
    delayedActions_.pop();
  }

  for (auto delayedAction : actionsToExecute) {
    auto action = delayedAction.action;
    auto playerId = delayedAction.playerId;

    spdlog::debug("Popped delayed action {0} at game tick {1}", action->getDescription(), *gameTicks_);

    auto delayedActionRewards = executeAndRecord(playerId, action);
    accumulateRewards(delayedRewards, delayedActionRewards);
  }

  return delayedRewards;
}

std::unordered_map<uint32_t, int32_t> Grid::processCollisions() {
  std::unordered_map<uint32_t, int32_t> collisionRewards;

  if (collisionDetectors_.size() == 0) {
    return collisionRewards;
  }

  // Check for collisions
  for (auto object : objects_) {
    auto objectName = object->getObjectName();
    auto collisionActionNamesIt = collisionSourceObjectActionNames_.find(objectName);
    if (collisionActionNamesIt != collisionSourceObjectActionNames_.end()) {
      auto collisionActionNames = collisionActionNamesIt->second;
      auto location = object->getLocation();
      auto playerId = object->getPlayerId();

      for (const auto& actionName : collisionActionNames) {
        spdlog::debug("Collision detector under action {0} for moved object {1} being queried", actionName, objectName);
        auto collisionDetector = collisionDetectors_.at(actionName);
        auto objectsInCollisionRange = collisionDetector->search(location);
        auto& actionTriggerDefinition = actionTriggerDefinitions_.at(actionName);

        for (auto collisionObject : objectsInCollisionRange) {
          if (collisionObject == object) continue;

          spdlog::debug("Collision detected for action {0} {1}->{2}", actionName, collisionObject->getObjectName(), objectName);

          std::shared_ptr<Action> collisionAction = std::shared_ptr<Action>(new Action(shared_from_this(), actionName, playerId, 0));
          collisionAction->init(object, collisionObject);

          executeAndRecord(0, collisionAction);
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

  accumulateRewards(rewards, delayedActionRewards);

  auto collisionRewards = processCollisions();
  accumulateRewards(rewards, collisionRewards);

  return rewards;
}

VectorPriorityQueue<DelayedActionQueueItem> Grid::getDelayedActions() {
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
  } else {
    return i->second;
  }
}

std::shared_ptr<Object> Grid::getObject(glm::ivec2 location) const {
  auto i = occupiedLocations_.find(location);

  if (i != occupiedLocations_.end()) {
    auto objectsAtLocation = i->second;
    if (objectsAtLocation.size() > 0) {
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

  for (auto& objectIdIt : objectIds_) {
    auto name = objectIdIt.first;
    auto idx = objectIdIt.second;
    orderedNames[idx] = name;
  }

  return orderedNames;
}

const std::vector<std::string> Grid::getObjectVariableNames() const {
  auto namesCount = objectVariableIds_.size();
  std::vector<std::string> orderedNames(namesCount);

  for (auto& objectVariableIdIt : objectVariableIds_) {
    auto name = objectVariableIdIt.first;
    auto idx = objectVariableIdIt.second;
    orderedNames[idx] = name;
  }

  return orderedNames;
}

void Grid::initObject(std::string objectName, std::vector<std::string> variableNames) {
  objectIds_.insert({objectName, objectIds_.size()});

  for (auto& variableName : variableNames) {
    objectVariableIds_.insert({variableName, objectVariableIds_.size()});
  }
}

std::unordered_map<uint32_t, std::shared_ptr<int32_t>> Grid::getObjectCounter(std::string objectName) {
  auto objectCounterIt = objectCounters_.find(objectName);
  if (objectCounterIt == objectCounters_.end()) {
    objectCounters_[objectName][0] = std::make_shared<int32_t>(0);
    return objectCounters_.at(objectName);
  }

  return objectCounterIt->second;
}

const std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>& Grid::getGlobalVariables() const {
  return globalVariables_;
}

void Grid::addActionProbability(std::string actionName, float probability) {
  actionProbabilities_[actionName] = probability;
}

void Grid::addActionTrigger(std::string actionName, ActionTriggerDefinition actionTriggerDefinition) {
  std::shared_ptr<CollisionDetector> collisionDetector = collisionDetectorFactory_->newCollisionDetector(width_, height_, actionTriggerDefinition);

  for (auto sourceObjectName : actionTriggerDefinition.sourceObjectNames) {
    collisionObjectActionNames_[sourceObjectName].insert(actionName);
    collisionSourceObjectActionNames_[sourceObjectName].insert(actionName);
  }

  for (auto destinationObjectName : actionTriggerDefinition.destinationObjectNames) {
    collisionObjectActionNames_[destinationObjectName].insert(actionName);
  }

  actionTriggerDefinitions_.insert({actionName, actionTriggerDefinition});
  collisionDetectors_.insert({actionName, collisionDetector});
}

void Grid::addPlayerDefaultObject(std::shared_ptr<Object> object) {
  spdlog::debug("Adding default object for player {0}", object->getPlayerId());

  object->init({-1, -1}, shared_from_this());

  defaultObject_[object->getPlayerId()] = object;
}

std::shared_ptr<Object> Grid::getPlayerDefaultObject(uint32_t playerId) const {
  spdlog::debug("Getting default object for player {0}", playerId);
  return defaultObject_.at(playerId);
}

void Grid::addObject(glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions) {
  auto objectName = object->getObjectName();
  auto playerId = object->getPlayerId();

  if (object->isPlayerAvatar()) {
    // If there is no playerId set on the object, we should set the playerId to 1 as 0 is reserved
    spdlog::debug("Player {3} avatar (playerId:{4}) set as object={0} at location [{1}, {2}]", object->getObjectName(), location.x, location.y, playerId);
    playerAvatars_[playerId] = object;
  }

  spdlog::debug("Adding object={0} belonging to player {1} to location: [{2},{3}]", objectName, playerId, location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->init(location, shared_from_this());

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
      auto initialActions = object->getInitialActions();
      if (initialActions.size() > 0) {
        spdlog::debug("Performing {0} Initial actions on object {1}.", initialActions.size(), objectName);
        performActions(0, initialActions);
      }
    }

    if (collisionDetectors_.size() > 0) {
      auto collisionDetectorActionNamesIt = collisionObjectActionNames_.find(objectName);
      if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
        auto collisionDetectorActionNames = collisionDetectorActionNamesIt->second;
        for (const auto& actionName : collisionDetectorActionNames) {
          auto collisionDetector = collisionDetectors_.at(actionName);
          collisionDetector->upsert(object);
        }
      }
    }

  } else {
    spdlog::error("Cannot add object={0} to location: [{1},{2}]", objectName, location.x, location.y);
  }
}

bool Grid::removeObject(std::shared_ptr<Object> object) {
  auto objectName = object->getObjectName();
  auto playerId = object->getPlayerId();
  auto location = object->getLocation();
  auto objectZIdx = object->getZIdx();
  spdlog::debug("Removing object={0} with playerId={1} from environment.", object->getDescription(), playerId);

  if (objects_.erase(object) > 0 && occupiedLocations_[location].erase(objectZIdx) > 0) {
    *objectCounters_[objectName][playerId] -= 1;
    invalidateLocation(location);

    // if we are removing a player's avatar
    if (playerAvatars_.size() > 0 && playerId != 0 && playerAvatars_.at(playerId) == object) {
      spdlog::debug("Removing player {0} avatar {1}", playerId, objectName);
      playerAvatars_.erase(playerId);
    }

    if (collisionDetectors_.size() > 0) {
      auto collisionDetectorActionNamesIt = collisionObjectActionNames_.find(objectName);
      if (collisionDetectorActionNamesIt != collisionObjectActionNames_.end()) {
        auto collisionDetectorActionNames = collisionDetectorActionNamesIt->second;
        for (const auto& actionName : collisionDetectorActionNames) {
          auto collisionDetector = collisionDetectors_.at(actionName);
          collisionDetector->remove(object);
        }
      }
    }

    return true;
  } else {
    spdlog::error("Could not remove object={0} from environment.", object->getDescription());
    return false;
  }
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

Grid::~Grid() {}
}  // namespace griddly