#include "Grid.hpp"

#include <spdlog/spdlog.h>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace griddy {

Grid::Grid() {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  gameTick = 0;
}

void Grid::init(uint32_t width, uint32_t height) {
  spdlog::debug("Setting grid dimensions to: [{0}, {1}]", width, height);
  height_ = height;
  width_ = width;

  occupiedLocations_.clear();
  objects_.clear();
  objectCounters_.clear();
}

bool Grid::updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation) {
  if (newLocation.x < 0 || newLocation.x >= width_ || newLocation.y < 0 || newLocation.y >= height_) {
    return false;
  }

  auto objectZIdx = object->getZIdx();

  occupiedLocations_[previousLocation].erase(objectZIdx);
  occupiedLocations_[newLocation][objectZIdx] = object;

  updatedLocations_.insert(previousLocation);
  updatedLocations_.insert(newLocation);

  return true;
}

std::unordered_set<GridLocation, GridLocation::Hash> Grid::getUpdatedLocations() const {
  return updatedLocations_;
}

std::vector<int> Grid::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  std::vector<int> rewards;

  // Reset the locations that need to be updated
  updatedLocations_.clear();

  spdlog::trace("Tick {0}", gameTick);

  for (auto action : actions) {
    auto sourceObject = getObject(action->getSourceLocation());
    auto destinationObject = getObject(action->getDestinationLocation());

    spdlog::debug("Player={0} performing action=({1})", playerId, action->getDescription());

    if (sourceObject == nullptr) {
      spdlog::debug("Cannot perform action on empty space.");
      rewards.push_back(0);
      continue;
    }

    auto sourceObjectPlayerId = sourceObject->getPlayerId();

    if (playerId != 0 && sourceObjectPlayerId != playerId) {
      spdlog::debug("Cannot perform action on objects not owned by player.");
      rewards.push_back(0);
      continue;
    }

    if (sourceObject->canPerformAction(action->getActionName())) {
      int reward = 0;
      if (destinationObject != nullptr) {
        auto dstBehaviourResult = destinationObject->onActionDst(sourceObject, action);
        reward += dstBehaviourResult.reward;

        if (dstBehaviourResult.abortAction) {
          rewards.push_back(reward);
          continue;
        }
      }

      auto srcBehaviourResult = sourceObject->onActionSrc(destinationObject, action);
      reward += srcBehaviourResult.reward;

      rewards.push_back(reward);

    } else {
      spdlog::debug("Cannot perform action={0} on object={1}", action->getActionName(), sourceObject->getObjectName());
      rewards.push_back(0);
    }
  }

  return rewards;
}

void Grid::update() {
  gameTick++;
}

uint32_t Grid::getTickCount() const {
  return gameTick;
}

std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

TileObjects Grid::getObjectsAt(GridLocation location) const {
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return {};
  } else {
    return i->second;
  }
}

std::shared_ptr<Object> Grid::getObject(GridLocation location) const {
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

std::unordered_map<uint32_t, std::shared_ptr<int32_t>> Grid::getObjectCounter(std::string objectName) const {
  auto objectCountIt = objectCounters_.find(objectName);
  if (objectCountIt == objectCounters_.end()) {
    return {};
  } else {
    return objectCountIt->second;
  }
}

void Grid::initObject(uint32_t playerId, GridLocation location, std::shared_ptr<Object> object) {
  auto objectName = object->getObjectName();
  spdlog::debug("Adding object={0} to location: [{1},{2}]", objectName, location.x, location.y);

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
      if(objectCounterForPlayerIt == objectCountersForPlayers.end()) {
        objectCounters_[objectName][playerId] = std::make_shared<int32_t>(0);
      }

      *objectCounters_[objectName][playerId] += 1;
      objectsAtLocation.insert({objectZIdx, object});
    }
  }
}

bool Grid::removeObject(std::shared_ptr<Object> object) {
  auto objectName = object->getObjectName();
  spdlog::debug("Removing object={0} from environment.", object->getDescription());

  if (objects_.erase(object) > 0 && occupiedLocations_.erase(object->getLocation()) > 0) {
    auto playerId = object->getPlayerId();
    *objectCounters_[objectName][playerId] -= 1;
    return true;
  } else {
    spdlog::error("Could not remove object={0} from environment.", object->getDescription());
    return false;
  }
}

uint32_t Grid::getWidth() const { return width_; }

uint32_t Grid::getHeight() const { return height_; }

Grid::~Grid() {}
}  // namespace griddy