#pragma once
#include <memory>

#include "../Grid.hpp"

namespace griddy {
class LevelGenerator {
 public:
  // Resets the grid based on the level generator implementation
  virtual void reset(std::shared_ptr<Grid> grid) = 0;

  virtual ~LevelGenerator() = 0;
};
}  // namespace griddy