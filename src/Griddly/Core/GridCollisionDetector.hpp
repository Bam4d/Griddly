#pragma once

#include <string>
#include <unordered_set>

#include "CollisionDetector.hpp"
#include "Grid.hpp"

namespace griddly {

class Grid;

// Collision detector that reads directly from the grid's occupancy map
// (Grid::getObjectsAt) instead of maintaining a separate spatial hash.
// The grid occupancy is the single source of truth for locations. The Grid
// tells the detector which object *types* it was added for (addObjectNames)
// and search only returns objects of those types - like the spatial hash it
// replaced, which only ever held objects of those types. Without that filter A* `Search: TargetObjectName`
// found the searching object itself (distance 0) and never moved, and triggers
// fired against every object type in range.
class GridCollisionDetector : public CollisionDetector {
 public:
  GridCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range, TriggerType triggerType);

  bool upsert(std::shared_ptr<ObjectRules> object) override { return true; }
  bool remove(std::shared_ptr<ObjectRules> object) override { return true; }

  // Object names this detector matches (called by Grid::addCollisionDetector).
  void addObjectNames(const std::unordered_set<std::string>& objectNames) { objectNames_.insert(objectNames.begin(), objectNames.end()); }

  void setGrid(Grid* grid) override { grid_ = grid; }

  SearchResult search(glm::ivec2 location) override;

 private:
  const TriggerType triggerType_;
  Grid* grid_ = nullptr;
  std::unordered_set<std::string> objectNames_;
};

}  // namespace griddly
