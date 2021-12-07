#include "CollisionDetectorFactory.hpp"

#include "SpatialHashCollisionDetector.hpp"

namespace griddly {

std::shared_ptr<CollisionDetector> CollisionDetectorFactory::newCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, ActionTriggerDefinition actionTriggerDefinition) {
  // Calculate bucket size
  auto minDim = gridWidth > gridHeight ? gridWidth : gridHeight;
  uint32_t cellSize = 10;

  if (minDim >= 100) {
    cellSize = (uint32_t)std::floor(std::sqrt((double)minDim));
  }

  return std::make_shared<SpatialHashCollisionDetector>(SpatialHashCollisionDetector(gridWidth, gridHeight, cellSize, actionTriggerDefinition.range, actionTriggerDefinition.triggerType));
}
}  // namespace griddly