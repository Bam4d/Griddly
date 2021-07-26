#include "Griddly/Core/CollisionDetectorFactory.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockCollisionDetectorFactory : public CollisionDetectorFactory {
 public:
  MockCollisionDetectorFactory() : CollisionDetectorFactory() {}
  ~MockCollisionDetectorFactory() {}

  MOCK_METHOD(std::shared_ptr<CollisionDetector>, newCollisionDetector, (uint32_t gridHeight, uint32_t gridWidth, ActionTriggerDefinition actionTriggerDefinition), ());
};
}