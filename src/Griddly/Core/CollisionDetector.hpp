#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

namespace griddly {

class Object;

class CollisionDetector {
 public:
  CollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range);

  virtual bool upsert(std::shared_ptr<Object> object) = 0;

  virtual bool remove(std::shared_ptr<Object> object) = 0;

  virtual std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) = 0;

 protected:
  const uint32_t range_;
  const uint32_t gridWidth_;
  const uint32_t gridHeight_;
};

}  // namespace griddly