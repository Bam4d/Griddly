#include "SpatialHashCollisionDetector.hpp"

#include <spdlog/spdlog.h>
#include <glm/gtx/quaternion.hpp> // need this for length2 function

namespace griddly {

SpatialHashCollisionDetector::SpatialHashCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t cellSize, uint32_t range, TriggerType triggerType)
    : CollisionDetector(gridWidth, gridHeight, range), triggerType_(triggerType), cellSize_(cellSize) {
}

bool SpatialHashCollisionDetector::upsert(std::shared_ptr<Object> object) {
  bool isNewObject = !remove(object);
  insert(object);
  return isNewObject;
}

void SpatialHashCollisionDetector::insert(const std::shared_ptr<Object>& object) {
  auto location = object->getLocation();
  auto hash = calculateHash(location);

  spdlog::debug("object at location [{0},{1}] added to hash [{2},{3}].", location.x, location.y, hash.x, hash.y);

  if (buckets_.find(hash) == buckets_.end()) {
    buckets_.insert({hash, {object}});
  } else {
    buckets_[hash].insert(object);
  }

  hashes_[object] = hash;
}

bool SpatialHashCollisionDetector::remove(std::shared_ptr<Object> object) {
  auto hashIt = hashes_.find(object);

  if (hashIt == hashes_.end()) {
    return false;
  }

  auto bucketIt = buckets_.find(hashIt->second);

  if (bucketIt == buckets_.end()) {
    return false;
  }

  spdlog::debug("object {0} removed from hash [{1},{2}].", object->getObjectName(), hashIt->second.x, hashIt->second.y);

  hashes_.erase(object);
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
      hashes.emplace_back(hashx, hashy);
    }
  }

  std::unordered_set<std::shared_ptr<Object>> collidedObjects;
  std::priority_queue<CollisionTarget, std::vector<CollisionTarget>, SortCollisionTargets> closestObjects;

  for (const auto& hash : hashes) {
    auto objectSet = buckets_[hash];

    switch (triggerType_) {
      case TriggerType::RANGE_BOX_BOUNDARY: {
        for (const auto& object : objectSet) {
          const auto& collisionLocation = object->getLocation();
          auto distance = glm::length2(static_cast<glm::vec2>(collisionLocation-location));
          if (std::abs(location.x - collisionLocation.x) == range_ && std::abs(location.y - collisionLocation.y) <= range_) {
            spdlog::debug("Range collided object at ({0},{1}), source object at ({2},{3}), distance: {4}", collisionLocation.x, collisionLocation.y, location.x, location.y, distance);
            collidedObjects.insert(object);
            closestObjects.push({distance, object});
          } else if (std::abs(location.y - collisionLocation.y) == range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            spdlog::debug("Range collided object at ({0},{1}), source object at ({2},{3}), distance: {4}", collisionLocation.x, collisionLocation.y, location.x, location.y, distance);
            collidedObjects.insert(object);
            closestObjects.push({distance, object});
          }
        }
      } break;
      case TriggerType::RANGE_BOX_AREA: {
        for (const auto& object : objectSet) {
          const auto& collisionLocation = object->getLocation();
          auto distance = glm::length2(static_cast<glm::vec2>(collisionLocation-location));
          if (std::abs(location.y - collisionLocation.y) <= range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            spdlog::debug("Area collided object at ({0},{1}), source object at ({2},{3}), distance: {4}", collisionLocation.x, collisionLocation.y, location.x, location.y, distance);
            collidedObjects.insert(object);
            closestObjects.push({distance, object});
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

glm::ivec2 SpatialHashCollisionDetector::calculateHash(glm::ivec2 location) const {
  auto xHash = location.x / cellSize_;
  auto yHash = location.y / cellSize_;
  return {xHash, yHash};
}

}  // namespace griddly
