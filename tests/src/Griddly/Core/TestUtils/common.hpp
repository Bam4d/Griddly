
#pragma once
#include <unordered_map>

#include "Mocks/Griddly/Core/GDY/Actions/MockAction.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

std::shared_ptr<MockObject> static mockObject(std::string objectName = "object", uint32_t playerId = 1, uint32_t zidx = 0, glm::ivec2 location = {0, 0}, DiscreteOrientation orientation = DiscreteOrientation(), std::unordered_set<std::string> availableActionNames = {}, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables = {}) {
  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockObjectPtr, getPlayerId()).WillRepeatedly(Return(playerId));
  EXPECT_CALL(*mockObjectPtr, getObjectName()).WillRepeatedly(Return(objectName));
  EXPECT_CALL(*mockObjectPtr, getObjectOrientation()).WillRepeatedly(Return(orientation));
  EXPECT_CALL(*mockObjectPtr, getObjectRenderTileName()).WillRepeatedly(Return(objectName + std::to_string(0)));
  EXPECT_CALL(*mockObjectPtr, getZIdx()).WillRepeatedly(Return(zidx));
  EXPECT_CALL(*mockObjectPtr, getLocation()).WillRepeatedly(Return(location));
  EXPECT_CALL(*mockObjectPtr, getAvailableVariables()).WillRepeatedly(Return(availableVariables));
  EXPECT_CALL(*mockObjectPtr, getAvailableActionNames()).WillRepeatedly(Return(availableActionNames));

  return mockObjectPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(sourceObject));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(destObject));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destObject->getLocation() - sourceObject->getLocation()));

  return mockActionPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, glm::ivec2 sourceLocation, glm::ivec2 destLocation) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceLocation));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destLocation));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destLocation - sourceLocation));

  return mockActionPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, std::shared_ptr<Object> sourceObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(sourceObject));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destLocation));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destLocation - sourceObject->getLocation()));

  return mockActionPtr;
}

}  // namespace griddly