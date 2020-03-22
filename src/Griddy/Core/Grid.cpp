#include "Grid.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace griddy {

Grid::Grid(int width, int height) : width_(width), height_(height) {
  spdlog::debug("Width={0} Height={1}", width, height);

  gameTick = 0;
}

bool Grid::updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation) {
  occupiedLocations_.erase(previousLocation);
  occupiedLocations_.insert({newLocation, object});
}

std::vector<int> Grid::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  // Could be a unique_ptr?
  std::vector<int> rewards;

  spdlog::trace("Tick {0}", gameTick);
  for (auto const& action : actions) {
    auto sourceObject = getObject(action->getSourceLocation());
    auto destinationObject = getObject(action->getDestinationLocation());

    spdlog::debug("Player={0} performing action=({1})", playerId, action->getDescription());

    if (sourceObject == nullptr) {
      spdlog::trace("Cannot perform action on empty space.");
      return rewards;
    }

    if (sourceObject->canPerformAction(action)) {
      spdlog::trace("Action={0} can be performed by Unit={1}", action->getDescription(), sourceObject->getDescription());

      auto actionResult = destinationObject == nullptr ? true : destinationObject->onActionPerformed(sourceObject, action);

      if (actionResult) {
        spdlog::debug("Action={0} performed on Object={1}", action->getDescription(), sourceObject->getDescription());
        auto actionReward = sourceObject->onPerformAction(destinationObject, action);
        rewards.push_back(actionReward);
      } else {
        spdlog::trace("Action={0} cannot be performed on Object={1}", action->getDescription(), destinationObject->getDescription());
      }
    } else {
      spdlog::trace("Action={0} cannot be performed by Unit={1}", action->getDescription(), sourceObject->getDescription());
    }
  }

  return rewards;
}

void Grid::update() {
  gameTick++;
}

int Grid::getTickCount() const {
  return gameTick;
}

std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

std::shared_ptr<Object> Grid::getObject(GridLocation location) const {
  auto i = occupiedLocations_.find(location);
  if (i == occupiedLocations_.end()) {
    return nullptr;
  } else {
    return i->second;
  }
}

void Grid::initObject(GridLocation location, std::shared_ptr<Object> object) {
  spdlog::debug("Adding object={0} to location: [{1},{2}]", object->getObjectType(), location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->init(location, shared_from_this());
    auto canAddToLocation = occupiedLocations_.insert({location, object}).second;
    if (!canAddToLocation) {
      objects_.erase(object);
    }
  }
}

int Grid::getWidth() const { return width_; }

int Grid::getHeight() const { return height_; }

Grid::~Grid() {}
}  // namespace griddy