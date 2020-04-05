#pragma once
#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddy/Core/Observers/Observer.hpp"
#include "../../src/Griddy/Core/Observers/TileObserver.hpp"

namespace griddy {
enum ObserverType { NONE,
                    BLOCK_2D,
                    TILE };

std::shared_ptr<Observer> createObserver(ObserverType observerType, std::shared_ptr<Grid> grid) {
  switch (observerType) {
    case BLOCK_2D:
      return std::shared_ptr<BlockObserver>(new BlockObserver(grid, 10));
      break;
    case TILE:
      return std::shared_ptr<TileObserver>(new TileObserver(grid));
      break;
    case NONE:
      return nullptr;
    default:
      return nullptr;
  }
}
}  // namespace griddy