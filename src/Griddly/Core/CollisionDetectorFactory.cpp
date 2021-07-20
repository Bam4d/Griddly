#include "CollisionDetectorFactory.hpp"
#include "SpatialHashCollisionDetector.hpp"

namespace griddly {

std::shared_ptr<CollisionDetector> CollisionDetectorFactory::newCollisionDetector(uint32_t gridWidth, uint32_t gridHeight, ActionTriggerDefinition actionTriggerDefinition) {
  
  // Calculate bucket size
  auto minDim = gridWidth > gridHeight ? gridWidth : gridHeight;
  uint32_t cellSize = 1;
  
  if (minDim >= 9) {
    cellSize = (uint32_t)std::floor(std::sqrt((double)minDim));
  }

  return std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(cellSize, actionTriggerDefinition.range, actionTriggerDefinition.triggerType));
}
}