#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/FixedWall.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/PushableWall.hpp"
#include "../../src/Griddy/Core/Objects/Terrain/Minerals.hpp"
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
  Py_GridWrapper(uint width, uint height) : grid_(std::shared_ptr<Grid>(new Grid(width, height))) {
  }

  uint getWidth() const {
    return grid_->getWidth();
  };

  uint getHeight() const {
    return grid_->getHeight();
  }

  void addObject(int playerId, uint startX, uint startY, ObjectType type) {
    std::shared_ptr<Object> object;

    switch (type) {
      case HARVESTER:
        object = std::shared_ptr<Harvester>(new Harvester(playerId));
        break;
      case PUSHER:
        object = std::shared_ptr<Pusher>(new Pusher(playerId));
        break;
      case PUNCHER:
        object = std::shared_ptr<Puncher>(new Puncher(playerId));
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

    grid_->initObject({startX, startY}, object);
  }

  std::shared_ptr<Py_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_);

    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(grid_, globalObserver));
  }

 private:
  const std::shared_ptr<Grid> grid_;

  bool isBuilt_ = false;
};

}  // namespace griddy