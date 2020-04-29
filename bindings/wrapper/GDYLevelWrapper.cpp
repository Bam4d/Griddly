#pragma once

#include <sstream>

#include "../../src/Griddy/Core/GDY/GDYFactory.hpp"
#include "GridWrapper.cpp"

namespace griddy {

class Py_GDYLevelWrapper {
 public:
  Py_GDYLevelWrapper(std::shared_ptr<GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
  }

  std::shared_ptr<Py_GridWrapper> createLevel(uint width, uint height) {
    auto grid = std::shared_ptr<Grid>(new Grid());
    gdyFactory_->createLevel(width, height, grid);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(grid, gdyFactory_));
  }

  std::shared_ptr<Py_GridWrapper> loadLevel(uint level) {
    gdyFactory_->loadLevel(level);
    auto grid = std::shared_ptr<Grid>(new Grid());
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(grid, gdyFactory_));
  }

 private:
  const std::shared_ptr<GDYFactory> gdyFactory_;
};
}  // namespace griddy