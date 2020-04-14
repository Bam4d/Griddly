#pragma once

#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/LevelGenerators/LevelGenerator.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/FixedWall.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/Minerals.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/PushableWall.hpp"
#include "../../src/Griddy/Core/Objects/Units/Harvester.hpp"
#include "../../src/Griddy/Core/Objects/Units/Puncher.hpp"
#include "../../src/Griddy/Core/Objects/Units/Pusher.hpp"
#include "../../src/Griddy/Core/TurnBasedGameProcess.hpp"
#include "GameProcessWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddy {

class Py_GridWrapper {
 public:
  Py_GridWrapper(uint width, uint height) : grid_(std::shared_ptr<Grid>(new Grid())), levelGenerator_(nullptr) {
    grid_->init(width, height);
  }

  Py_GridWrapper(std::shared_ptr<LevelGenerator> levelGenerator) : grid_(std::shared_ptr<Grid>(new Grid())), levelGenerator_(levelGenerator) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  uint getWidth() const {
    return grid_->getWidth();
  }

  uint getHeight() const {
    return grid_->getHeight();
  }

  void addObject(int playerId, uint startX, uint startY, ObjectType type) {
    std::shared_ptr<Object> object;

    switch (type) {
      case HARVESTER:
        object = std::shared_ptr<Harvester>(new Harvester());
        break;
      case PUSHER:
        object = std::shared_ptr<Pusher>(new Pusher());
        break;
      case PUNCHER:
        object = std::shared_ptr<Puncher>(new Puncher());
        break;
      case FIXED_WALL:
        object = std::shared_ptr<FixedWall>(new FixedWall());
        break;
      case PUSHABLE_WALL:
        object = std::shared_ptr<PushableWall>(new PushableWall());
        break;
      case MINERALS:
        object = std::shared_ptr<Minerals>(new Minerals(10));
        break;
      default:
        break;
    }

    grid_->initObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Py_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_);

    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(grid_, globalObserver, levelGenerator_));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  const std::shared_ptr<LevelGenerator> levelGenerator_;

  bool isBuilt_ = false;
};

}  // namespace griddy