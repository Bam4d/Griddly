#pragma once

#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddy/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/TurnBasedGameProcess.hpp"
#include "GameProcessWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddy {

class Py_GridWrapper {
 public:

  Py_GridWrapper( std::shared_ptr<Grid> grid, std::shared_ptr<GDYFactory> gdyFactory) : grid_(grid), gdyFactory_(gdyFactory) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  uint32_t getWidth() const {
    return grid_->getWidth();
  }

  uint32_t getHeight() const {
    return grid_->getHeight();
  }

  void addObject(int playerId, uint32_t startX, uint32_t startY, std::string objectName) {

    auto objectGenerator = gdyFactory_->getObjectGenerator();

    auto object = objectGenerator->newInstance(objectName);

    grid_->initObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Py_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_, gdyFactory_);

    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(grid_, globalObserver, gdyFactory_));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  const std::shared_ptr<GDYFactory> gdyFactory_;

  bool isBuilt_ = false;
};

}  // namespace griddy