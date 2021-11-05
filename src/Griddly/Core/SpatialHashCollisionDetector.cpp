#include "SpatialHashCollisionDetector.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

SpatialHashCollisionDetector::SpatialHashCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t cellSize, uint32_t range, TriggerType triggerType)
    : CollisionDetector(gridWidth, gridHeight, range), triggerType_(triggerType), cellSize_(cellSize) {
}

bool SpatialHashCollisionDetector::upsert(std::shared_ptr<Object> object) {
  bool isNewObject = !remove(object);
  insert(object);
  return isNewObject;
}

void SpatialHashCollisionDetector::insert(std::shared_ptr<Object> object) {
  auto location = object->getLocation();
  auto hash = calculateHash(location);

  spdlog::debug("object at location [{0},{1}] added to hash [{2},{3}].", location.x, location.y, hash.x, hash.y);

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

  spdlog::debug("object at location [{0},{1}] removed from hash [{2},{3}].", location.x, location.y, hash.x, hash.y);

  return bucketIt->second.erase(object) > 0;
}

SearchResult SpatialHashCollisionDetector::search(glm::ivec2 location) {
  auto top = std::min(gridHeight_, location.y + range_);
  auto bottom = std::max(0, location.y - static_cast<int32_t>(range_));

  auto right = std::min(gridWidth_, location.x + range_);
  auto left = std::max(0, location.x - static_cast<int32_t>(range_));

  auto bottomLeft = calculateHash(glm::ivec2(left, bottom));
  auto topRight = calculateHash(glm::ivec2(right, top));

  std::vector<glm::ivec2> hashes;
  for (uint32_t hashy = bottomLeft.y; hashy <= topRight.y; hashy++) {
    for (uint32_t hashx = bottomLeft.x; hashx <= topRight.x; hashx++) {
      hashes.push_back({hashx, hashy});
    }
  }

  std::unordered_set<std::shared_ptr<Object>> collidedObjects;
  std::vector<std::shared_ptr<Object>> closestObjects;

  for (const auto& hash : hashes) {
    auto objectSet = buckets_[hash];

    switch (triggerType_) {
      case TriggerType::RANGE_BOX_BOUNDARY: {
        for (auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if (std::abs(location.x - collisionLocation.x) == range_ && std::abs(location.y - collisionLocation.y) <= range_) {
            spdlog::debug("Range collided object at ({0},{1}), source object at ({2},{3})", collisionLocation.x, collisionLocation.y, location.x, location.y);
            collidedObjects.insert(object);
          } else if (std::abs(location.y - collisionLocation.y) == range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            spdlog::debug("Range collided object at ({0},{1}), source object at ({2},{3})", collisionLocation.x, collisionLocation.y, location.x, location.y);
            collidedObjects.insert(object);
            closestObjects.push_back(object);
          }
        }
      } break;
      case TriggerType::RANGE_BOX_AREA: {
        for (auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if (std::abs(location.y - collisionLocation.y) <= range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            spdlog::debug("Area collided object at ({0},{1}), source object at ({2},{3})", collisionLocation.x, collisionLocation.y, location.x, location.y);
            collidedObjects.insert(object);
            closestObjects.push_back(object);
          }
        }
      } break;
      case TriggerType::NONE:
        throw std::invalid_argument("Misconfigured collision detector!, specify 'RANGE_BOX_BOUNDARY' or 'RANGE_BOX_AREA' in configuration");
        break;
    }
  }

  return {collidedObjects, closestObjects};
}

glm::ivec2 SpatialHashCollisionDetector::calculateHash(glm::ivec2 location) {
  auto xHash = location.x / cellSize_;
  auto yHash = location.y / cellSize_;
  return {xHash, yHash};
}

}  // namespace griddly
