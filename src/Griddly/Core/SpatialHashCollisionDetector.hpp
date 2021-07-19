#include "CollisionDetector.hpp"

namespace griddly {

class SpatialHashCollisionDetector : public CollisionDetector {
 public:
  SpatialHashCollisionDetector(uint32_t numBuckets, uint32_t range, std::string actionName, TriggerType triggerType);

  virtual void updateLocation(std::shared_ptr<Object> object) override;

  virtual std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) override;

 private:
  glm::ivec2 calculateHash(glm::ivec2 location);

  void insert(std::shared_ptr<Object> object);

  void remove(std::shared_ptr<Object> object);

  std::unordered_map<glm::ivec2, std::unordered_set<std::shared_ptr<Object>>> buckets_;

  const uint32_t numBuckets_;
};

}  // namespace griddly
