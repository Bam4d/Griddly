#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_set>

namespace griddly {

class Object;


struct SearchResult {
  std::unordered_set<std::shared_ptr<Object>> objectSet;
  std::vector<std::shared_ptr<Object>> closestObjects;
};

class CollisionDetector {
 public:
  CollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range);

  virtual bool upsert(std::shared_ptr<Object> object) = 0;

  virtual bool remove(std::shared_ptr<Object> object) = 0;

  virtual SearchResult search(glm::ivec2 location) = 0;

 protected:
  const uint32_t range_;
  const uint32_t gridWidth_;
  const uint32_t gridHeight_;
};

}  // namespace griddly