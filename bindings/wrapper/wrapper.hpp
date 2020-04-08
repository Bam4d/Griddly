#pragma once
#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddy/Core/Observers/Observer.hpp"
#include "../../src/Griddy/Core/Observers/SpriteObserver.hpp"
#include "../../src/Griddy/Core/Observers/TileObserver.hpp"

namespace griddy {
enum ObserverType { NONE,
                    SPRITE_2D,
                    BLOCK_2D,
                    VECTOR };

std::shared_ptr<Observer> createObserver(ObserverType observerType, std::shared_ptr<Grid> grid) {
  switch (observerType) {
    case SPRITE_2D:
      return std::shared_ptr<SpriteObserver>(new SpriteObserver(grid, 16));
      break;
    case BLOCK_2D : return std::shared_ptr<BlockObserver>(new BlockObserver(grid, 10));
      break;
    case VECTOR:
      return std::shared_ptr<TileObserver>(new TileObserver(grid));
      break;
    case NONE:
      return nullptr;
    default:
      return nullptr;
  }
}
}  // namespace griddy