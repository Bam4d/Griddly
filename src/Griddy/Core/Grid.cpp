#include "Grid.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <vector>

namespace griddy {

Grid::Grid(int width, int height) : width_(width), height_(height) {
  spdlog::debug("Width: {0} Height: {1}", width, height);
}

void Grid::update(std::vector<std::shared_ptr<Action>> actions) {
  for (auto const& action : actions) {
    spdlog::debug("Action: {0}", action->log());
  }
}

Grid::~Grid() {}
}  // namespace griddy