#pragma once

#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockAction : public Action {
 public:
  MockAction()
      : Action(nullptr, "mockAction", 0) {}

  MOCK_METHOD(void, init, (glm::ivec2 sourceLocation, glm::ivec2 destinationLocation), ());
  MOCK_METHOD(void, init, (std::shared_ptr<Object> sourceObject, glm::ivec2 vectorToDest, glm::ivec2 orientationVector, bool relativeToSource), ());

  MOCK_METHOD(std::shared_ptr<Object>, getSourceObject, (), (const));
  MOCK_METHOD(std::shared_ptr<Object>, getDestinationObject, (), (const));

  MOCK_METHOD(glm::ivec2, getSourceLocation, (), (const));
  MOCK_METHOD(glm::ivec2, getDestinationLocation, (), (const));

  MOCK_METHOD(glm::ivec2, getVectorToDest, (), (const));
  MOCK_METHOD(glm::ivec2, getOrientationVector, (), (const));

  MOCK_METHOD(std::string, getActionName, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(uint32_t, getDelay, (), (const));
};
}  // namespace griddly