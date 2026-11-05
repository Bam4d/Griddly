#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>
#include <queue>

namespace griddly {

class ObjectRules;
class Grid;


struct CollisionTarget {
  float distance;
  std::shared_ptr<ObjectRules> target;
};

struct SortCollisionTargets {
  bool operator()(const CollisionTarget& a, const CollisionTarget& b) {
    return a.distance > b.distance;
  };
};

struct SearchResult {
  std::unordered_set<std::shared_ptr<ObjectRules>> objectSet;
  std::priority_queue<CollisionTarget, std::vector<CollisionTarget>, SortCollisionTargets> closestObjects;
};

class CollisionDetector {
 public:
  CollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range);
  virtual ~CollisionDetector() = default;

  virtual bool upsert(std::shared_ptr<ObjectRules> object) = 0;

  virtual bool remove(std::shared_ptr<ObjectRules> object) = 0;

  virtual SearchResult search(glm::ivec2 location) = 0;

  // The grid whose occupancy map this detector reads (set by Grid).
  virtual void setGrid(Grid* grid) {}

 protected:
  const uint32_t range_;
  const uint32_t gridWidth_;
  const uint32_t gridHeight_;
};

}  // namespace griddly