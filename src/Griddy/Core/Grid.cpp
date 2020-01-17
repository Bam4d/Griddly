#include "Grid.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <vector>

namespace griddy {

Grid::Grid(int width, int height) : width_(width), height_(height) {
  spdlog::debug("Width={0} Height={1}", width, height);
}

std::vector<int> Grid::performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) {
  
  // Could be a unique_ptr?
  std::vector<int> rewards;
  
  for (auto const& action : actions) {
    spdlog::debug("Player={0} Action={1}", playerId, action->getDescription());
  }

  return rewards;
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
  spdlog::debug("Adding object={0} to location: [{1},{2}]", object->getType(), location.x, location.y);

  auto canAddObject = objects_.insert(object).second;
  if (canAddObject) {
    object->setLocation(location);
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