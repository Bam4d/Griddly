#include <memory>

#include "GDY/Objects/Object.hpp"
#include "Grid.hpp"

namespace griddly {

class CollisionDetector {
 public:
  CollisionDetector(uint32_t range, std::string actionName, TriggerType triggerType);

  virtual bool upsert(std::shared_ptr<Object> object) = 0;

  virtual bool remove(std::shared_ptr<Object> object) = 0;

  virtual std::unordered_set<std::shared_ptr<Object>> search(glm::ivec2 location) = 0;

 protected:
  const std::string actionName_;
  const uint32_t range_;
  const TriggerType triggerType_;
};

}  // namespace griddly