#include "SpatialHashCollisionDetector.hpp"

namespace griddly {

SpatialHashCollisionDetector::SpatialHashCollisionDetector(uint32_t cellSize, uint32_t range, std::string actionName, TriggerType triggerType)
    : CollisionDetector(range, actionName, triggerType), cellSize_(cellSize) {
}

bool SpatialHashCollisionDetector::upsert(std::shared_ptr<Object> object) {
  bool isNewObject = !remove(object);
  insert(object);
  return isNewObject;
}

void SpatialHashCollisionDetector::insert(std::shared_ptr<Object> object) {
  auto location = object->getLocation();
  auto hash = calculateHash(location);

  if (buckets_.find(hash) == buckets_.end()) {
    buckets_.insert({hash, {object}});
  } else {
    buckets_[hash].insert(object);
  }
}

bool SpatialHashCollisionDetector::remove(std::shared_ptr<Object> object) {
  auto location = object->getLocation();
  auto hash = calculateHash(location);
  auto bucketIt = buckets_.find(hash);
  if (bucketIt == buckets_.end()) {
    return false;
  }

  return bucketIt->second.erase(object) > 0;
}

std::unordered_set<std::shared_ptr<Object>> SpatialHashCollisionDetector::search(glm::ivec2 location) {
  auto topLeft = glm::ivec2(location.x - range_, location.y - range_);
  auto bottomLeft = glm::ivec2(location.x - range_, location.y + range_);
  auto topRight = glm::ivec2(location.x + range_, location.y - range_);
  auto bottomRight = glm::ivec2(location.x + range_, location.y + range_);

  const std::unordered_set<glm::ivec2> hashes = {
      calculateHash(topLeft),
      calculateHash(bottomLeft),
      calculateHash(topRight),
      calculateHash(bottomRight),
  };

  std::unordered_set<std::shared_ptr<Object>> collidedObjects;

  for (const auto& hash : hashes) {
    auto objectSet = buckets_[hash];

    switch (triggerType_) {
      case TriggerType::RANGE_BOX_BOUNDARY: {
        for (auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if (std::abs(location.x - collisionLocation.x) == range_ && std::abs(location.y - collisionLocation.y) <= range_) {
            collidedObjects.insert(object);
          } else if (std::abs(location.y - collisionLocation.y) == range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            collidedObjects.insert(object);
          }
        }
      }
      break;
      case TriggerType::RANGE_BOX_AREA: {
        for (auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if (std::abs(location.y - collisionLocation.y) <= range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            collidedObjects.insert(object);
          }
        }
      }
      break;
    }
  }

  return collidedObjects;
}

glm::ivec2 SpatialHashCollisionDetector::calculateHash(glm::ivec2 location) {
  auto xHash = location.x / cellSize_;
  auto yHash = location.y / cellSize_;
  return {xHash, yHash};
}

}  // namespace griddly
