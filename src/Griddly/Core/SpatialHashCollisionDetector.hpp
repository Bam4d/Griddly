#pragma once

#include <unordered_map>

#include "CollisionDetector.hpp"
#include "Grid.hpp"

namespace griddly {

class SpatialHashCollisionDetector : public CollisionDetector {
 public:
  SpatialHashCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t cellSize, uint32_t range, TriggerType triggerType);

  bool upsert(std::shared_ptr<Object> object) override;

  bool remove(std::shared_ptr<Object> object) override;

  SearchResult search(glm::ivec2 location) override;

 private:
  glm::ivec2 calculateHash(glm::ivec2 location) const;

  void insert(const std::shared_ptr<Object>& object);

  std::unordered_map<glm::ivec2, std::unordered_set<std::shared_ptr<Object>>> buckets_ = {};
  std::unordered_map<std::shared_ptr<Object>, glm::ivec2> hashes_ = {};

  const uint32_t cellSize_;
  const TriggerType triggerType_;
};

}  // namespace griddly
