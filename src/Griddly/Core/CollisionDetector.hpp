#pragma once

#include <memory>
#include <unordered_set>

#include <glm/glm.hpp>

namespace griddly {

class Object;

class CollisionDetector {
 public:
  CollisionDetector(uint32_t range);

  virtual bool upsert(std::shared_ptr<Object> object) = 0;

  virtual bool remove(std::shared_ptr<Object> object) = 0;

  virtual std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) = 0;

 protected:
  const uint32_t range_;
};

}  // namespace griddly