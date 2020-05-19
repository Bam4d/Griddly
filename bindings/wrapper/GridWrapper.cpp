#pragma once

#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddle/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddle/Core/Grid.hpp"
#include "../../src/Griddle/Core/TurnBasedGameProcess.hpp"
#include "GameProcessWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddle {

class Py_GridWrapper {
 public:

  Py_GridWrapper( std::shared_ptr<Grid> grid, std::shared_ptr<GDYFactory> gdyFactory, std::string resourceLocation) : grid_(grid), gdyFactory_(gdyFactory), resourceLocation_(resourceLocation) {
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

    auto object = objectGenerator->newInstance(objectName, grid_->getGlobalParameters());

    grid_->initObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Py_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_, gdyFactory_, resourceLocation_);
    
    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(grid_, globalObserver, gdyFactory_, resourceLocation_));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::string resourceLocation_;

  bool isBuilt_ = false;
};

}  // namespace griddle