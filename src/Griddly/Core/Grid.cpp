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
}

void Grid::resetMap(uint32_t width, uint32_t height) {
  spdlog::debug("Setting grid dimensions to: [{0}, {1}]", width, height);
  height_ = height;
  width_ = width;

  occupiedLocations_.clear();
  objects_.clear();
  objectCounters_.clear();
  objectNames_.clear();
  delayedActions_ = {};

  gameTicks_ = std::make_shared<int32_t>(0);
}

void Grid::resetGlobalVariables(std::unordered_map<std::string, int32_t> globalVariableDefinitions) {
  globalVariables_.clear();
  for (auto variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto variableInitialValue = std::make_shared<int32_t>(variable.second);
    globalVariables_.insert({variableName, variableInitialValue});
  }
}

bool Grid::invalidateLocation(glm::ivec2 location) {
  updatedLocations_.insert(location);
  return true;
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

  updatedLocations_.insert(previousLocation);
  updatedLocations_.insert(newLocation);

  return true;
}

std::unordered_set<glm::ivec2> Grid::getUpdatedLocations() const {
  return updatedLocations_;
}

int32_t Grid::executeAndRecord(uint32_t playerId, std::shared_ptr<Action> action) {
  if (recordEvents_) {
    auto event = buildGridEvent(action, playerId, *gameTicks_);
    auto reward = executeAction(playerId, action);
    recordGridEvent(event, reward);
    return reward;
  } else {
    return executeAction(playerId, action);
  }
}

int32_t Grid::executeAction(uint32_t playerId, std::shared_ptr<Action> action) {
  auto sourceObject = action->getSourceObject();
  auto destinationObject = action->getDestinationObject();

  spdlog::debug("Executing action {0}", action->getDescription());

  if (sourceObject == nullptr) {
    spdlog::debug("Cannot perform action on empty space.");
    return 0;
  }

  auto sourceObjectPlayerId = sourceObject->getPlayerId();

  if (playerId != 0 && sourceObjectPlayerId != playerId) {
    spdlog::debug("Cannot perform action on object not owned by player. Object owner {0}, Player owner {1}", sourceObjectPlayerId, playerId);
    return 0;
  }

  if (sourceObject->isValidAction(action)) {
    int reward = 0;
    if (destinationObject != nullptr && destinationObject.get() != sourceObject.get()) {
      auto dstBehaviourResult = destinationObject->onActionDst(action);
      reward += dstBehaviourResult.reward;

      if (dstBehaviourResult.abortAction) {
        spdlog::debug("Action {0} aborted by destination object behaviour.", action->getDescription());
        return reward;
      }
    }

    auto srcBehaviourResult = sourceObject->onActionSrc(action);
    reward += srcBehaviourResult.reward;
    return reward;

  } else {
    spdlog::debug("Cannot perform action={0} on object={1}", action->getActionName(), sourceObject->getObjectName());
    return 0;
  }
}

GridEvent Grid::buildGridEvent(std::shared_ptr<Action> action, uint32_t playerId, uint32_t tick) {
  auto sourceObject = action->getSourceObject();
  auto destObject = action->getDestinationObject();

  GridEvent event;
  event.playerId = playerId;
  event.actionName = action->getActionName();

  if (sourceObject != nullptr) {
    event.sourceObjectPlayerId = sourceObject->getPlayerId();
    event.sourceObjectName = sourceObject->getObjectName();
  } else {
    event.sourceObjectName = "_empty";
  }

  if (destObject != nullptr) {
    event.destinationObjectPlayerId = destObject->getPlayerId();
    event.destObjectName = destObject->getObjectName();
  } else {
    event.destObjectName = "_empty";
  }

  event.sourceLocation = action->getSourceLocation();
  event.destLocation = action->getDestinationLocation();

  event.tick = tick;
  event.delay = action->getDelay();

  return event;
}

void Grid::recordGridEvent(GridEvent event, int32_t reward) {
  event.reward = reward;
  eventHistory_.push_back(event);
}

std::vector<int> Grid::performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  std::vector<int> rewards;

  // Reset the locations that need to be updated
  // ! We want the rendering to be consistent across all players so only reset
  // ! on one of the players so the other players still render OK
  if (playerId == 1) {
    updatedLocations_.clear();
  }

  spdlog::trace("Tick {0}", *gameTicks_);

  for (auto action : actions) {
    // Check if action is delayed or durative
    if (action->getDelay() > 0) {
      delayAction(playerId, action);
    } else {
      rewards.push_back(executeAndRecord(playerId, action));
    }
  }

  return rewards;
}

void Grid::delayAction(uint32_t playerId, std::shared_ptr<Action> action) {
  auto executionTarget = *(gameTicks_) + action->getDelay();
  spdlog::debug("Delaying action={0} to execution target time {1}", action->getDescription(), executionTarget);
  delayedActions_.push(DelayedActionQueueItem{playerId, executionTarget, action});
}

std::unordered_map<uint32_t, int32_t> Grid::update() {
  *(gameTicks_) += 1;

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

    delayedRewards[playerId] += executeAndRecord(playerId, action);
  }

  return delayedRewards;
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

std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

TileObjects Grid::getObjectsAt(glm::ivec2 location) const {
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return {};
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

uint32_t Grid::getUniqueObjectCount() const {
  return objectNames_.size();
}

void Grid::initObject(std::string objectName) {
  objectNames_.insert(objectName);
}

std::unordered_map<uint32_t, std::shared_ptr<int32_t>> Grid::getObjectCounter(std::string objectName) {
  auto objectCounterIt = objectCounters_.find(objectName);
  if (objectCounterIt == objectCounters_.end()) {
    objectCounters_[objectName][0] = std::make_shared<int32_t>(0);
    return objectCounters_.at(objectName);
  }

  return objectCounterIt->second;
}

std::unordered_map<std::string, std::shared_ptr<int32_t>> Grid::getGlobalVariables() const {
  return globalVariables_;
}

void Grid::addObject(uint32_t playerId, glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions) {
  auto objectName = object->getObjectName();

  if (object->isPlayerAvatar()) {
    // If there is no playerId set on the object, we should set the playerId to 1 as 0 is reserved
    if (playerId == 0) {
      playerId = 1;
    }

    spdlog::debug("Player {3} avatar ( playerId:{4}) set as object={0} at location [{1}, {2}]", object->getObjectName(), location.x, location.y, playerId);
    playerAvatars_[playerId] = object;
  }

  spdlog::debug("Adding object={0} belonging to player {1} to location: [{2},{3}]", objectName, playerId, location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->init(playerId, location, shared_from_this());

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
      updatedLocations_.insert(location);
    }

    if (applyInitialActions) {
      auto initialActions = object->getInitialActions();
      if (initialActions.size() > 0) {
        spdlog::debug("Performing {0} Initial actions on object {1}.", initialActions.size(), objectName);
        performActions(0, initialActions);
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
    updatedLocations_.insert(location);
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

std::vector<GridEvent> Grid::getHistory() const {
  return eventHistory_;
}

void Grid::purgeHistory() {
  eventHistory_.clear();
}

Grid::~Grid() {}
}  // namespace griddly