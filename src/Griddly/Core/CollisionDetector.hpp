#include <memory>

#include "GDY/Objects/Object.hpp"
#include "Grid.hpp"

namespace griddly {

class CollisionDetector {
 public:
  CollisionDetector(uint32_t range, std::string actionName, TriggerType triggerType);

  virtual void updateLocation(std::shared_ptr<Object> object) = 0;

  virtual std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) = 0;

 private:
  const std::string actionName_;
  const uint32_t range_;
  const TriggerType triggerType_;
};

}  // namespace griddly