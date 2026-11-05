#include "GridCollisionDetector.hpp"

#include <cmath>

namespace griddly {

GridCollisionDetector::GridCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range, TriggerType triggerType)
    : CollisionDetector(gridWidth, gridHeight, range), triggerType_(triggerType) {
}

SearchResult GridCollisionDetector::search(glm::ivec2 location) {
  // Direct tile lookup: scan the (2*range+1)^2 tiles around the location in
  // the grid occupancy map. For the typical range=1 that is 9 map lookups.
  auto top = std::min((int32_t)gridHeight_, location.y + (int32_t)range_);
  auto bottom = std::max(0, location.y - (int32_t)range_);
  auto right = std::min((int32_t)gridWidth_, location.x + (int32_t)range_);
  auto left = std::max(0, location.x - (int32_t)range_);

  std::unordered_set<std::shared_ptr<ObjectRules>> collidedObjects;
  std::priority_queue<CollisionTarget, std::vector<CollisionTarget>, SortCollisionTargets> closestObjects;

  for (int32_t y = bottom; y <= top; y++) {
    for (int32_t x = left; x <= right; x++) {
      const auto& tileObjects = grid_->getObjectsAt({x, y});
      for (const auto& zAndObject : tileObjects) {
        const auto& object = zAndObject.second;
        const auto& collisionLocation = object->getLocation();
        auto dx = std::abs(location.x - collisionLocation.x);
        auto dy = std::abs(location.y - collisionLocation.y);

        bool hit = false;
        switch (triggerType_) {
          case TriggerType::RANGE_BOX_BOUNDARY:
            hit = (dx == (int32_t)range_ && dy <= (int32_t)range_) || (dy == (int32_t)range_ && dx <= (int32_t)range_);
            break;
          case TriggerType::RANGE_BOX_AREA:
            hit = dx <= (int32_t)range_ && dy <= (int32_t)range_;
            break;
          case TriggerType::NONE:
            throw std::invalid_argument("Misconfigured collision detector!, specify 'RANGE_BOX_BOUNDARY' or 'RANGE_BOX_AREA' in configuration");
        }

        if (hit) {
          auto d = collisionLocation - location;
          auto distance = (float)(d.x * d.x + d.y * d.y);
          collidedObjects.insert(object);
          closestObjects.push(CollisionTarget{distance, object});
        }
      }
    }
  }

  return {collidedObjects, closestObjects};
}

}  // namespace griddly
