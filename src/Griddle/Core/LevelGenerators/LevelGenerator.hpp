#pragma once
#include <memory>

#include "../Grid.hpp"

namespace griddle {
class LevelGenerator {
 public:
  // Resets the grid based on the level generator implementation
  virtual std::unordered_map<uint32_t, std::shared_ptr<Object>> reset(std::shared_ptr<Grid> grid) = 0;

  virtual ~LevelGenerator() = 0;
};
}  // namespace griddle