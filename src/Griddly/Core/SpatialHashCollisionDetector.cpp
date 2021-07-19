#include <memory>

#include "GDY/Objects/Object.hpp"
#include "Grid.hpp"

namespace griddly {

SpatialHashCollisionDetector::SpatialHashCollisionDetector(uint32_t numBuckets, uint32_t range, std::string actionName, TriggerType triggerType)
    : CollisionDetector(range, actionName, triggerType), numBuckets_(numBuckets) {
}

void SpatialHashCollisionDetector::updateLocation(std::shared_ptr<Object> object) {
  remove(object);
  insert(object);
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

void SpatialHashCollisionDetector::remove(std::shared_ptr<Object> object) {
  auto location = object->getLocation();
  auto hash = calculateHash(location);
  buckets_[hash].erase(object);
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
      case TriggerType::RANGE_BOX_AREA: {
        for (auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if (std::abs(location.y - collisionLocation.y) <= range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            collidedObjects.insert(object);
          }
        }
      }
    }
  }

  return collidedObjects;
}

glm::ivec2 SpatialHashCollisionDetector::calculateHash(glm::ivec2 location) {
  auto xHash = location.x / numBuckets_;
  auto yHash = location.y / numBuckets_;
  return {xHash, yHash};
}

}  // namespace griddly
