#pragma once

#include "CollisionDetector.hpp"
#include "Grid.hpp"

namespace griddly {

class Grid;

// Collision detector that reads directly from the grid's occupancy map
// (Grid::getObjectsAt) instead of maintaining a separate spatial hash.
// upsert/remove are no-ops: the grid occupancy is the single source of truth.
class GridCollisionDetector : public CollisionDetector {
 public:
  GridCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range, TriggerType triggerType);

  bool upsert(std::shared_ptr<ObjectRules> object) override { return true; }
  bool remove(std::shared_ptr<ObjectRules> object) override { return true; }

  void setGrid(Grid* grid) override { grid_ = grid; }

  SearchResult search(glm::ivec2 location) override;

 private:
  const TriggerType triggerType_;
  Grid* grid_ = nullptr;
};

}  // namespace griddly
