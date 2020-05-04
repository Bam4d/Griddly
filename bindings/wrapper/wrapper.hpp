#pragma once
#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddy/Core/Observers/Observer.hpp"
#include "../../src/Griddy/Core/Observers/SpriteObserver.hpp"
#include "../../src/Griddy/Core/Observers/TileObserver.hpp"

namespace griddy {

enum class ObserverType { NONE,
                          SPRITE_2D,
                          BLOCK_2D,
                          VECTOR };

std::shared_ptr<Observer> createObserver(ObserverType observerType,
                                         std::shared_ptr<Grid> grid,
                                         std::shared_ptr<GDYFactory> gdyFactory) {
  switch (observerType) {
    case ObserverType::SPRITE_2D:
      return std::shared_ptr<SpriteObserver>(new SpriteObserver(grid, gdyFactory->getTileSize(), gdyFactory->getSpriteObserverDefinitions()));
      break;
    case ObserverType::BLOCK_2D:
      return std::shared_ptr<BlockObserver>(new BlockObserver(grid, gdyFactory->getTileSize(), gdyFactory->getBlockObserverDefinitions()));
      break;
    case ObserverType::VECTOR:
      return std::shared_ptr<TileObserver>(new TileObserver(grid));
      break;
    case ObserverType::NONE:
      return nullptr;
    default:
      return nullptr;
  }
}
}  // namespace griddy