#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>
#include <queue>

namespace griddly {

class Object;


struct CollisionTarget {
  float distance;
  std::shared_ptr<Object> target;
};

struct SortCollisionTargets {
  bool operator()(const CollisionTarget& a, const CollisionTarget& b) {
    return a.distance > b.distance;
  };
};

struct SearchResult {
  std::unordered_set<std::shared_ptr<Object>> objectSet;
  std::priority_queue<CollisionTarget, std::vector<CollisionTarget>, SortCollisionTargets> closestObjects;
};

class CollisionDetector {
 public:
  CollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range);
  virtual ~CollisionDetector() = default;

  virtual bool upsert(std::shared_ptr<Object> object) = 0;

  virtual bool remove(std::shared_ptr<Object> object) = 0;

  virtual SearchResult search(glm::ivec2 location) = 0;

 protected:
  const uint32_t range_;
  const uint32_t gridWidth_;
  const uint32_t gridHeight_;
};

}  // namespace griddly