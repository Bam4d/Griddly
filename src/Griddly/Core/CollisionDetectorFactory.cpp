#include "CollisionDetectorFactory.hpp"

#include "GridCollisionDetector.hpp"

namespace griddly {

std::shared_ptr<CollisionDetector> CollisionDetectorFactory::newCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, ActionTriggerDefinition actionTriggerDefinition) {
  // Direct grid-occupancy lookup; no separate spatial hash to maintain.
  return std::make_shared<GridCollisionDetector>(gridWidth, gridHeight, actionTriggerDefinition.range, actionTriggerDefinition.triggerType);
}
}  // namespace griddly
