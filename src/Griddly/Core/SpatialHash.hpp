#include<memory>
#include "GDY/Objects/Object.hpp"

namespace griddly {

class SpatialHash {
 public:

  SpatialHash(uint32_t gridWidth, uint32_t gridHeight, )

  void insert(std::shared_ptr<Object> object) {
    auto location = object->getLocation();
    auto hash = getHash(location);

    if(buckets_.find(hash) == buckets_.end()) {
      buckets_.insert({hash, {object}});
    } else {
      buckets_[hash].insert(object);
    }
  }

  void remove(std::shared_ptr<Object> object) {
    auto location = object->getLocation();
    auto hash = getHash(location);
    buckets_[hash].remove(object);
  }

  std::unordered_set<std::shared_ptr<Object>> search(int32_t range, glm::ivec2 location) {

    auto topLeft = glm::ivec2(location.x - range, location.y - range);
    auto bottomLeft = glm::ivec2(location.x - range, location.y + range);
    auto topRight = glm::ivec2(location.x + range, location.y - range);
    auto bottomRight = glm::ivec2(location.x + range, location.y + range);

    const std::unordered_set<int32_t> hashes = {
      hash(topLeft),
      hash(bottomLeft),
      hash(topRight),
      hash(bottomRight),
    };

    std::unordered_set<std::shared_ptr<Object>> possibleObjectsInRange;

    for (const auto& hash: hashes) {

      auto objectSet = buckets_[hash];
      possibleObjectsInRange.insert(objectSet.begin(), objectSet.end());
    }

    return possibleObjectsInRange;
  }


  private:
    
    glm::ivec3 hash(std::shared_ptr<Object> object) {
      auto xHash = location.x / bucketSize_;
      auto yHash = location.y / bucketSize_;
      return {xHash, yHash}
    }

    std::unordered_map<glm::ivec2, std::unordered_set<std::shared_ptr<Object>>> buckets_;

    uint32_t bucketSize_;
};

}  // namespace griddly
