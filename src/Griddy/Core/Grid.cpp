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

std::vector<std::shared_ptr<Object>>& Grid::getObjects() {
  return this->objects;
}

void Grid::initObject(GridLocation location, std::shared_ptr<Object> object) {
  
  spdlog::debug("Adding object={0} to location: [{1},{2}]", object->getType(), location.x, location.y);

  object->setLocation(location);
  objects.push_back(std::move(object));
}

int Grid::getWidth() {
  return width_;
}

int Grid::getHeight() {
  return height_;
}

Grid::~Grid() {}
}  // namespace griddy