#include "CollisionDetector.hpp"

namespace griddly {

CollisionDetector::CollisionDetector(uint32_t range, std::string actionName, TriggerType triggerType)
    : range_(range), actionName_(actionName), triggerType_(triggerType) {
}

}  // namespace griddly