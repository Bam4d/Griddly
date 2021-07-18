#include <memory>

#include "GDY/Objects/Object.hpp"
#include "Grid.hpp"

namespace griddly {

class SpatialHashCollisionDetector {
 public:
  SpatialHashCollisionDetector(uint32_t numBuckets, uint32_t range, std::string actionName, TriggerType triggerType)
      : numBuckets_(numBuckets), range_(range), actionName_(actionName), triggerType_(triggerType) {
  }

  void insert(std::shared_ptr<Object> object) {
    auto location = object->getLocation();
    auto hash = calculateHash(location);

    if (buckets_.find(hash) == buckets_.end()) {
      buckets_.insert({hash, {object}});
    } else {
      buckets_[hash].insert(object);
    }
  }

  void remove(std::shared_ptr<Object> object) {
    auto location = object->getLocation();
    auto hash = calculateHash(location);
    buckets_[hash].erase(object);
  }

  std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) {
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

      if(triggerType_ == TriggerType::RANGE_BOX_AREA) {
        for(auto object : objectSet) {
          auto collisionLocation = object->getLocation();
          if(std::abs(location.x - collisionLocation.x) == range_ && std::abs(location.y - collisionLocation.y) <= range_) {
            collidedObjects.insert(object);
          } else if(std::abs(location.y - collisionLocation.y) == range_ && std::abs(location.x - collisionLocation.x) <= range_) {
            collidedObjects.insert(object);
          }
        }
      }

      
    }

    return collidedObjects;
  }

  std::string getActionName() const {
    return actionName_;
  }

 private:
  glm::ivec2 calculateHash(glm::ivec2 location) {
    auto xHash = location.x / numBuckets_;
    auto yHash = location.y / numBuckets_;
    return {xHash, yHash};
  }

  std::unordered_map<glm::ivec2, std::unordered_set<std::shared_ptr<Object>>> buckets_;

  const std::string actionName_;
  const uint32_t numBuckets_;
  const uint32_t range_;
  const TriggerType triggerType_;
};

}  // namespace griddly
