
#pragma once
#include <memory>
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
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;

namespace griddly {

std::shared_ptr<MockObject> static mockObject(std::string objectName = "object", char mapCharacter = '?', uint32_t playerId = 1, uint32_t zidx = 0, const glm::ivec2 location = {0, 0}, DiscreteOrientation orientation = DiscreteOrientation(), std::unordered_set<std::string> availableActionNames = {}, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables = {}) {
  auto mockObjectPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockObjectPtr, getPlayerId()).WillRepeatedly(Return(playerId));
  EXPECT_CALL(*mockObjectPtr, getObjectName()).WillRepeatedly(ReturnRefOfCopy(objectName));
  EXPECT_CALL(*mockObjectPtr, getMapCharacter()).WillRepeatedly(Return(mapCharacter));
  EXPECT_CALL(*mockObjectPtr, getObjectOrientation()).WillRepeatedly(Return(orientation));
  EXPECT_CALL(*mockObjectPtr, getObjectRenderTileName()).WillRepeatedly(ReturnRefOfCopy(objectName + std::to_string(0)));
  EXPECT_CALL(*mockObjectPtr, getZIdx()).WillRepeatedly(Return(zidx));
  EXPECT_CALL(*mockObjectPtr, getLocation()).WillRepeatedly(ReturnRefOfCopy(location));
  EXPECT_CALL(*mockObjectPtr, getAvailableVariables()).WillRepeatedly(Return(availableVariables));
  EXPECT_CALL(*mockObjectPtr, getAvailableActionNames()).WillRepeatedly(Return(availableActionNames));

  ON_CALL(*mockObjectPtr, getVariableValue).WillByDefault(Return(nullptr));

  for(auto variable : availableVariables) {
    ON_CALL(*mockObjectPtr, getVariableValue(Eq(variable.first))).WillByDefault(Return(variable.second));
  }

  return mockObjectPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(sourceObject));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(destObject));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destObject->getLocation() - sourceObject->getLocation()));

  return mockActionPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, glm::ivec2 sourceLocation, glm::ivec2 destLocation) {
  auto mockActionPtr = std::make_shared<MockAction>();

  const std::string empty = "_empty";

  auto mockDefaultObject = std::make_shared<MockObject>();
  EXPECT_CALL(*mockDefaultObject, getObjectName()).WillRepeatedly(ReturnRefOfCopy(empty));

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(mockDefaultObject));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(mockDefaultObject));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceLocation));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destLocation));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destLocation - sourceLocation));

  return mockActionPtr;
}

std::shared_ptr<MockAction> static mockAction(std::string actionName, std::shared_ptr<Object> sourceObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::make_shared<MockAction>();

  const std::string empty = "_empty";

  auto mockDefaultObject = std::make_shared<MockObject>();
  EXPECT_CALL(*mockDefaultObject, getObjectName()).WillRepeatedly(ReturnRefOfCopy(empty));

  EXPECT_CALL(*mockActionPtr, getActionName()).WillRepeatedly(Return(actionName));
  EXPECT_CALL(*mockActionPtr, getSourceObject()).WillRepeatedly(Return(sourceObject));
  EXPECT_CALL(*mockActionPtr, getDestinationObject()).WillRepeatedly(Return(mockDefaultObject));
  EXPECT_CALL(*mockActionPtr, getSourceLocation()).WillRepeatedly(Return(sourceObject->getLocation()));
  EXPECT_CALL(*mockActionPtr, getDestinationLocation()).WillRepeatedly(Return(destLocation));
  EXPECT_CALL(*mockActionPtr, getVectorToDest()).WillRepeatedly(Return(destLocation - sourceObject->getLocation()));

  return mockActionPtr;
}

bool static commandArgumentsEqual(CommandArguments a, CommandArguments b) {
  for (auto& it : a) {
    auto key = it.first;
    auto node = it.second;

    if (node.Type() != b[key].Type()) {
      return false;
    }
  }
  return true;
}

bool static commandListEqual(std::vector<std::pair<std::string, CommandArguments>> a, std::vector<std::pair<std::string, CommandArguments>> b) {

  for(int i = 0; i<a.size(); i++) {
    auto pairA = a[i];
    auto pairB = b[i];

    if(pairA.first != pairB.first) {
      return false;
    }

    if(!commandArgumentsEqual(pairA.second, pairB.second)) {
      return false;
    }
  }
  return true;
}

MATCHER_P(ActionBehaviourDefinitionEqMatcher, behaviour, "") {
  auto isEqual = behaviour.behaviourType == arg.behaviourType &&
                 behaviour.sourceObjectName == arg.sourceObjectName &&
                 behaviour.destinationObjectName == arg.destinationObjectName &&
                 behaviour.actionName == arg.actionName &&
                 behaviour.commandName == arg.commandName &&
                 behaviour.executionProbability == arg.executionProbability &&
                 commandArgumentsEqual(behaviour.commandArguments, arg.commandArguments) &&
                 commandListEqual(behaviour.conditionalCommands, arg.conditionalCommands);

  return isEqual;
}

}  // namespace griddly