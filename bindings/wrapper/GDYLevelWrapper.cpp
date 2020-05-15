#pragma once

#include <sstream>

#include "../../src/Griddle/Core/GDY/GDYFactory.hpp"
#include "GridWrapper.cpp"

namespace griddle {

class Py_GDYLevelWrapper {
 public:
  Py_GDYLevelWrapper(std::shared_ptr<GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
  }

  std::shared_ptr<Py_GridWrapper> createLevel(uint32_t width, uint32_t height) {
    auto grid = std::shared_ptr<Grid>(new Grid());
    gdyFactory_->createLevel(width, height, grid);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(grid, gdyFactory_));
  }

  std::shared_ptr<Py_GridWrapper> loadLevel(uint32_t level) {
    gdyFactory_->loadLevel(level);
    auto grid = std::shared_ptr<Grid>(new Grid());
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(grid, gdyFactory_));
  }

  std::shared_ptr<Py_GridWrapper> loadLevelString(std::string levelString) {
    gdyFactory_->loadLevelString(levelString);
    auto grid = std::shared_ptr<Grid>(new Grid());
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(grid, gdyFactory_));
  }

 private:
  const std::shared_ptr<GDYFactory> gdyFactory_;
};
}  // namespace griddle