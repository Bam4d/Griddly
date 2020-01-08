#include "Grid.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <vector>

namespace griddy {

Grid::Grid(int width, int height) : width_(width), height_(height) {
  spdlog::debug("Width={0} Height={1}", width, height);
}

void Grid::update(std::vector<std::shared_ptr<Action>> actions) {
  for (auto const& action : actions) {
    spdlog::debug("Action={0}", action->log());
  }
}

std::unordered_set<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects_;
}

std::shared_ptr<Object> Grid::getObject(GridLocation location) {
  return occupiedLocations_[location];
}

void Grid::initObject(GridLocation location, std::shared_ptr<Object> object) {
  
  spdlog::debug("Adding object={0} to location: [{1},{2}]", object->getType(), location.x, location.y);

  object->setLocation(location);
  occupiedLocations_[location] = object;
  objects_.insert(object);
}

int Grid::getWidth() {
  return width_;
}

int Grid::getHeight() {
  return height_;
}

Grid::~Grid() {}
}  // namespace griddy