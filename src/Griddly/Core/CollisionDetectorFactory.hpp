#pragma once

#include <memory>

#include "CollisionDetector.hpp"

namespace griddly {

struct ActionTriggerDefinition;

class CollisionDetectorFactory {
 public:
  virtual ~CollisionDetectorFactory() = default;
  virtual std::shared_ptr<CollisionDetector> newCollisionDetector(uint32_t gridHeight, uint32_t gridWidth, ActionTriggerDefinition actionTriggerDefinition);
};

}  // namespace griddly