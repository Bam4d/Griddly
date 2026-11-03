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

  // (Re)create the ECS state block sized for this level. One allocation,
  // never resized afterwards.
  ECSWorldConfig ecsConfig;
  ecsConfig.gridWidth = width;
  ecsConfig.gridHeight = height;
  ecsConfig.maxEntities = width * height * 4 + 16;  // allow multiple objects per tile
  ecsConfig.maxPlayers = playerCount_ + 1;
  ecsConfig.maxGlobalVariables = 64;
  ecsConfig.maxVariablesPerEntity = 32;
  ecsConfig.maxDelayedActions = 256;
  ecsWorld_ = std::make_shared<ECSWorld>(ecsConfig);
  objects_.clear();
  objects_.reserve(ecsConfig.maxEntities);
  occupancyDirty_ = true;

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

  if (ecsWorld_) {
    ecsWorld_->clear();
  }
}

void Grid::setGlobalVariables(const std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>>& globalVariableDefinitions) {
  globalVariables_.clear();
  globalVariableIds_.clear();
  for (const auto& variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto playerVariables = variable.second;
    globalVariableIds_[variableName] = globalVariableIds_.size();

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
  globalVariableIds_.clear();
  for (const auto& variable : globalVariableDefinitions) {
    auto variableName = variable.first;
    auto variableDefinition = variable.second;
    globalVariableIds_[variableName] = globalVariableIds_.size();

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

bool Grid::updateLocation(std::shared_ptr<ObjectRules> object, glm::ivec2 previousLocation, glm::ivec2 newLocation) {
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
  occupancyDirty_ = false;

  invalidateLocation(previousLocation);
  invalidateLocation(newLocation);

  // Mirror the move into the ECS state block (arena x/y become the source
  // of truth here; the object's own location_ is updated by the caller).
  if (ecsWorld_) {
    ecsWorld_->moveEntity(object->getEntityId(), newLocation.x, newLocation.y);
  }

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
  return toRewardMap(executeAndRecordArr(playerId, action));
}

RewardArray Grid::executeAndRecordArr(uint32_t playerId, const std::shared_ptr<Action>& action) {
  auto reward = executeActionArr(playerId, action);
  if (recordEvents_) {
    auto event = buildGridEvent(action, playerId, *gameTicks_);
    recordGridEvent(event, toRewardMap(reward));
  }
  return reward;
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
  return toRewardMap(executeActionArr(playerId, action));
}

RewardArray Grid::executeActionArr(uint32_t playerId, const std::shared_ptr<Action>& action) {
  auto sourceObject = action->getSourceObject();

  if (!(ecsWorld_ && sourceObject && ecsWorld_->isAlive(sourceObject->getEntityId())) && action->getDelay() > 0) {
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
    spdlog::debug("Cannot perform action on object not owned by player. ObjectRules owner {0}, Player owner {1}", sourceObjectPlayerId, playerId);
    return {};
  }

  if (playerId != 0 && sourceObject->isPlayerAvatar() && playerAvatars_.find(playerId) == playerAvatars_.end()) {
    spdlog::debug("Avatar for player {0} has been removed, action will be ignored.", playerId);
    return {};
  }

  auto validBehaviourIdxs = sourceObject->getValidBehaviourIdxs(action);

  auto filteredBehaviourIdxs = filterBehaviourProbabilities(validBehaviourIdxs, behaviourProbabilities_.at(action->getActionName()));

  if (filteredBehaviourIdxs.size() > 0) {
    RewardArray rewardAccumulator{};
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
  return toRewardMap(performActionsArr(playerId, actions));
}

RewardArray Grid::performActionsArr(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) {
  RewardArray rewardAccumulator{};

  spdlog::trace("Tick {0}", *gameTicks_);

  // Have to add some non-determinism here for multi-agent games so player 1 doesn't always benefit
  std::shuffle(std::begin(actions), std::end(actions), randomGenerator_->getEngine());

  for (const auto& action : actions) {
    // Check if action is delayed or durative
    if (action->getDelay() > 0) {
      delayAction(playerId, action);
    } else {
      auto actionRewards = executeAndRecordArr(playerId, action);
      accumulateRewards(rewardAccumulator, actionRewards);
    }
  }

  return rewardAccumulator;
}

void Grid::delayAction(uint32_t playerId, std::shared_ptr<Action> action) {
  auto executionTarget = *(gameTicks_) + action->getDelay();
  spdlog::debug("Delaying action={0} to execution target time {1}", action->getDescription(), executionTarget);
  delayedActions_.push(std::make_shared<DelayedActionQueueItem>(DelayedActionQueueItem{playerId, executionTarget, action}));

  // Mirror into the arena so save/load/reset/clone include queued events.
  if (ecsWorld_) {
    const auto& actionName = action->getActionName();
    const uint32_t hash = actionNameHash(actionName);
    actionHashToName_[hash] = actionName;
    auto source = action->getSourceObject();
    DelayedActionRecord record{};
    record.tick = executionTarget;
    record.playerId = playerId;
    record.sourceEntity = source ? source->getEntityId() : NULL_ENTITY;
    record.actionId = hash;
    record.vectorToDestX = action->getVectorToDest().x;
    record.vectorToDestY = action->getVectorToDest().y;
    record.orientationX = action->getOrientationVector().x;
    record.orientationY = action->getOrientationVector().y;
    ecsWorld_->pushDelayedAction(record);
  }
}

void Grid::mirrorDelayedActionsToECS() {
  if (!ecsWorld_) return;
  ecsWorld_->clearDelayedActions();
  for (const auto& item : delayedActions_) {
    const auto& actionName = item->action->getActionName();
    const uint32_t hash = actionNameHash(actionName);
    actionHashToName_[hash] = actionName;
    auto source = item->action->getSourceObject();
    DelayedActionRecord record{};
    record.tick = item->priority;
    record.playerId = item->playerId;
    record.sourceEntity = source ? source->getEntityId() : NULL_ENTITY;
    record.actionId = hash;
    record.vectorToDestX = item->action->getVectorToDest().x;
    record.vectorToDestY = item->action->getVectorToDest().y;
    record.orientationX = item->action->getOrientationVector().x;
    record.orientationY = item->action->getOrientationVector().y;
    ecsWorld_->pushDelayedAction(record);
  }
}

void Grid::rebuildDelayedActionsFromECS() {
  if (!ecsWorld_) return;
  delayedActions_ = {};
  auto self = shared_from_this();
  const uint32_t count = ecsWorld_->delayedActionCount();
  for (uint32_t i = 0; i < count; ++i) {
    const auto& rec = ecsWorld_->delayedAction(i);
    auto nameIt = actionHashToName_.find(rec.actionId);
    if (nameIt == actionHashToName_.end()) continue;  // unknown action hash
    // Find the live object for the source entity slot.
    std::shared_ptr<ObjectRules> source;
    for (const auto& object : objects_) {
      if (object->getEntityId().index == rec.sourceEntity.index && object->getEntityId().generation == rec.sourceEntity.generation) {
        source = object;
        break;
      }
    }
    if (!source) continue;
    auto action = std::make_shared<Action>(Action(self, nameIt->second, rec.playerId, rec.tick > static_cast<uint32_t>(*gameTicks_) ? rec.tick - *gameTicks_ : 0));
    action->init(source, {rec.vectorToDestX, rec.vectorToDestY}, {rec.orientationX, rec.orientationY}, false);
    delayedActions_.push(std::make_shared<DelayedActionQueueItem>(DelayedActionQueueItem{rec.playerId, rec.tick, action}));
  }
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
  if (ecsWorld_) {
    ecsWorld_->setTick(tickCount);
  }
}

const std::vector<std::shared_ptr<ObjectRules>>& Grid::getObjects() {
  return this->objects_;
}

void Grid::rebuildOccupancyIfDirty() const {
  if (!occupancyDirty_ || !ecsWorld_) return;
  occupiedLocations_.clear();
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> bySlot;
  for (const auto& object : objects_) bySlot[object->getEntityId().index] = object;
  const auto& cfg = ecsWorld_->config();
  for (uint32_t ty = 0; ty < cfg.gridHeight; ++ty) {
    for (uint32_t tx = 0; tx < cfg.gridWidth; ++tx) {
      // walk the tile's z-ordered list in the arena
      auto head = ecsWorld_->topObjectAt(static_cast<int32_t>(tx), static_cast<int32_t>(ty));
      while (head != NULL_ENTITY) {
        auto it = bySlot.find(head.index);
        if (it != bySlot.end()) {
          occupiedLocations_[{static_cast<int32_t>(tx), static_cast<int32_t>(ty)}][it->second->zIdxValue()] = it->second;
        }
        head = ecsWorld_->nextAtLocation(head);
      }
    }
  }
  occupancyDirty_ = false;
}

const TileObjects& Grid::getObjectsAt(glm::ivec2 location) const {
  rebuildOccupancyIfDirty();
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return EMPTY_OBJECTS;
  }
  return i->second;
}

std::shared_ptr<ObjectRules> Grid::getObject(glm::ivec2 location) const {
  rebuildOccupancyIfDirty();
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

void Grid::addPlayerDefaultEmptyObject(std::shared_ptr<ObjectRules> emptyObject) {
  spdlog::debug("Adding default empty object for player {0}", emptyObject->getPlayerId());
  emptyObject->init({-1, -1});
  defaultEmptyObject_[emptyObject->getPlayerId()] = emptyObject;
}


void Grid::addPlayerDefaultBoundaryObject(std::shared_ptr<ObjectRules> boundaryObject) {
  spdlog::debug("Adding default boundary object for player {0}", boundaryObject->getPlayerId());
  boundaryObject->init({-1, -1});
  defaultBoundaryObject_[boundaryObject->getPlayerId()] = boundaryObject;
}

std::shared_ptr<ObjectRules> Grid::getPlayerDefaultEmptyObject(uint32_t playerId) const {
  spdlog::debug("Getting default empty object for player {0}", playerId);
  return defaultEmptyObject_.at(playerId);
}

std::shared_ptr<ObjectRules> Grid::getPlayerDefaultBoundaryObject(uint32_t playerId) const {
  spdlog::debug("Getting default boundary object for player {0}", playerId);
  return defaultBoundaryObject_.at(playerId);
}

void Grid::addObject(glm::ivec2 location, std::shared_ptr<ObjectRules> object, bool applyInitialActions, std::shared_ptr<Action> originatingAction, DiscreteOrientation orientation) {
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

  auto canAddObject = !(ecsWorld_ && ecsWorld_->isAlive(object->getEntityId()));
  if (canAddObject) objects_.push_back(object);
  if (canAddObject) {
    object->init(location, orientation);

    auto objectZIdx = object->getZIdx();
    auto& objectsAtLocation = occupiedLocations_[location];
    occupancyDirty_ = false;

    auto objectAtZIt = objectsAtLocation.find(objectZIdx);

    // If we find an in this location with the same zindex, do not add it.
    if (objectAtZIt != objectsAtLocation.end()) {
      spdlog::error("Cannot add object={0} to location: [{1},{2}], there is already an object here.", objectName, location.x, location.y);
      objects_.pop_back();
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

      // Create the ECS entity and bind the object's variable storage to the
      // arena slot, so all subsequent reads/writes land in the state block.
      if (ecsWorld_) {
        auto entity = ecsWorld_->createEntity();
        object->setEntityId(entity);
        ecsWorld_->objectTypeId(entity) = objectIds_.at(objectName);
        ecsWorld_->zIdx(entity) = objectZIdx;
        object->bindToECS(ecsWorld_.get());
        ecsWorld_->placeEntity(entity, location.x, location.y);
      }
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

bool Grid::removeObject(std::shared_ptr<ObjectRules> object) {
  const auto& objectName = object->getObjectName();
  auto playerId = object->getPlayerId();
  auto location = object->getLocation();
  auto objectZIdx = object->getZIdx();
  spdlog::debug("Removing object={0} with playerId={1} from environment.", object->getDescription(), playerId);

  bool erased = false;
  for (auto it = objects_.begin(); it != objects_.end(); ++it) {
    if (*it == object) { objects_.erase(it); erased = true; break; }
  }
  occupancyDirty_ = false;
  if (erased && occupiedLocations_[location].erase(objectZIdx) > 0) {
    *objectCounters_[objectName][playerId] -= 1;
    invalidateLocation(location);

    // Retire the ECS entity (slot is recycled, generation bumped).
    if (ecsWorld_) {
      ecsWorld_->destroyEntity(object->getEntityId());
    }

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

std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> Grid::getPlayerAvatarObjects() const {
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

// ----- ECS state block -----

std::shared_ptr<ECSWorld> Grid::getECSWorld() const {
  return ecsWorld_;
}

void Grid::syncObjectToECS(const std::shared_ptr<ObjectRules>& object) {
  if (!ecsWorld_) return;
  // Bound objects alias the arena directly, so there is nothing to copy —
  // their reads/writes already land in the state block.
  if (object->isECSBound()) return;
  const auto entity = object->getEntityId();
  if (!ecsWorld_->isAlive(entity)) return;

  // Unbound objects (not the normal path): sync scalar components only, via
  // non-virtual accessors so gmock test doubles are safe.
  const auto& location = object->locationRef();
  const auto& orientation = object->orientationRef();
  ecsWorld_->x(entity) = location.x;
  ecsWorld_->y(entity) = location.y;
  ecsWorld_->dx(entity) = *orientation.getDx();
  ecsWorld_->dy(entity) = *orientation.getDy();
  ecsWorld_->playerId(entity) = object->playerIdValue();
  ecsWorld_->renderTileId(entity) = object->renderTileIdValue();
}

void Grid::syncObjectsToECS() {
  if (!ecsWorld_) return;
  for (const auto& object : objects_) {
    syncObjectToECS(object);
  }
  const auto& cfg = ecsWorld_->config();
  ecsWorld_->setTick(*gameTicks_);
  mirrorDelayedActionsToECS();
  ecsWorld_->setGlobalVariableCount(globalVariableIds_.size());
  for (const auto& [name, perPlayer] : globalVariables_) {
    const auto idxIt = globalVariableIds_.find(name);
    if (idxIt == globalVariableIds_.end() || idxIt->second >= cfg.maxGlobalVariables) continue;
    for (const auto& [player, value] : perPlayer) {
      if (player < cfg.maxPlayers) {
        ecsWorld_->globalVariable(idxIt->second, player) = *value;
      }
    }
  }
}

void Grid::syncECSToObjects() {
  if (!ecsWorld_) return;

  // Index live objects by entity slot.
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> objectsBySlot;
  for (const auto& object : objects_) {
    objectsBySlot[object->getEntityId().index] = object;
  }

  // Occupancy is rebuilt lazily from the arena on first access.
  occupancyDirty_ = true;
  const auto& cfg = ecsWorld_->config();

  for (const auto& object : objects_) {
    const auto entity = object->getEntityId();
    if (!ecsWorld_->isAlive(entity)) continue;
    // Bound objects alias the arena, so a state restore (memcpy) is already
    // visible through their variable pointers; only the plain location_
    // member needs updating. Unbound objects have no arena slot to read.
    if (object->isECSBound()) {
      object->setLocationRef({ecsWorld_->x(entity), ecsWorld_->y(entity)});
    }
  }

  // Global variables back into the C++ side.
  for (auto& [name, perPlayer] : globalVariables_) {
    const auto idxIt = globalVariableIds_.find(name);
    if (idxIt == globalVariableIds_.end() || idxIt->second >= cfg.maxGlobalVariables) continue;
    for (auto& [player, value] : perPlayer) {
      if (player < cfg.maxPlayers) {
        *value = ecsWorld_->globalVariable(idxIt->second, player);
      }
    }
  }

  *gameTicks_ = ecsWorld_->tick();
  rebuildDelayedActionsFromECS();
}

Action* Grid::acquireAction(const std::string& actionName, uint32_t playerId, uint32_t delay) {
  Action* action;
  if (!freeActions_.empty()) {
    action = freeActions_.back();
    freeActions_.pop_back();
    action->~Action();
  } else {
    action = new Action(shared_from_this(), actionName, playerId, delay);  // one-time pool growth
    actionPool_.push_back(action);
    return action;
  }
  new (action) Action(shared_from_this(), actionName, playerId, delay);
  return action;
}

void Grid::releaseActions(const std::vector<Action*>& actions) {
  for (auto* action : actions) freeActions_.push_back(action);
}

std::unordered_map<uint32_t, int32_t> Grid::performActionsPooled(uint32_t playerId, const std::vector<Action*>& actions) {
  actionScratch_.clear();
  for (auto* action : actions) {
    actionScratch_.push_back(std::shared_ptr<Action>(noopAnchor_, action));  // aliasing: no control-block alloc
  }
  auto rewards = performActions(playerId, actionScratch_);
  releaseActions(actions);
  return rewards;
}

size_t Grid::saveStateTo(void* dst) {
  if (!ecsWorld_) return 0;
  syncObjectsToECS();
  return ecsWorld_->saveTo(dst);
}

bool Grid::loadStateFrom(const void* src, size_t size) {
  if (!ecsWorld_ || !ecsWorld_->loadFrom(src, size)) return false;
  syncECSToObjects();
  return true;
}

void Grid::captureInitialState() {
  if (!ecsWorld_) return;
  syncObjectsToECS();
  ecsWorld_->captureInitialState();
}

void Grid::resetToInitialState() {
  if (!ecsWorld_ || !ecsWorld_->hasInitialState()) {
    reset();
    return;
  }
  ecsWorld_->resetToInitial();  // one memcpy of the state block
  syncECSToObjects();
}

void Grid::cloneStateFrom(const Grid& src) {
  if (!src.ecsWorld_) return;
  // Fresh arena of the same shape, then one memcpy of the source state block.
  resetMap(src.width_, src.height_);
  const_cast<Grid&>(src).syncObjectsToECS();
  ecsWorld_->copyStateFrom(*src.ecsWorld_);

  // Share the ObjectRules graph: same instances, retargeted to this grid's
  // arena (offsets unchanged, only the base moves). No per-object rebuild.
  auto self = shared_from_this();
  objects_.clear();
  for (const auto& object : src.objects_) {
    object->rebindGrid(self);
    objects_.push_back(object);
  }
  occupancyDirty_ = true;

  // Rebuild the C++-side views from the arena.
  objectIds_ = src.objectIds_;
  objectVariableIds_ = src.objectVariableIds_;
  globalVariableIds_ = src.globalVariableIds_;
  globalVariables_ = src.globalVariables_;
  *gameTicks_ = ecsWorld_->tick();
  occupancyDirty_ = true;  // occupancy rebuilt lazily from the arena

  // Object counters from the arena.
  objectCounters_.clear();
  playerAvatars_.clear();
  std::unordered_map<uint32_t, std::string> typeToName;
  for (const auto& [n, id] : objectIds_) typeToName[id] = n;
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> slotToObject;
  for (const auto& object : objects_) slotToObject[object->getEntityId().index] = object;
  ecsWorld_->forEachAlive([&](EntityId e) {
    auto it = typeToName.find(ecsWorld_->objectTypeId(e));
    if (it != typeToName.end()) {
      auto& cell = objectCounters_[it->second][static_cast<uint32_t>(ecsWorld_->playerId(e))];
      if (!cell) cell = std::make_shared<int32_t>(0);
      *cell += 1;
    }
    if (ecsWorld_->isPlayerAvatar(e)) {
      auto objIt = slotToObject.find(e.index);
      if (objIt != slotToObject.end()) {
        playerAvatars_[static_cast<uint32_t>(ecsWorld_->playerId(e))] = objIt->second;
      }
    }
  });
  // Every known object type gets a player-0 counter entry even if none are
  // currently alive (termination conditions index counter.at(0)).
  for (const auto& [name, id] : objectIds_) {
    (void)id;
    auto& cell = objectCounters_[name][0];
    if (!cell) cell = std::make_shared<int32_t>(0);
  }

  actionHashToName_ = src.actionHashToName_;
  rebuildDelayedActionsFromECS();  // arena queue was copied by the memcpy
}

}  // namespace griddly