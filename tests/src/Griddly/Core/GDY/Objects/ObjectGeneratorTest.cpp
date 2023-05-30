#include <memory>
#include <unordered_map>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/TestUtils/common.hpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

#define _V(X) std::make_shared<int32_t>(X)
#define _Y(X) YAML::Node(X)

namespace griddly {
TEST(ObjectGeneratorTest, cloneInstance) {
  std::string objectAName = "objectA";
  std::string objectBName = "objectB";
  auto orientation = DiscreteOrientation(Direction::RIGHT);
  glm::ivec2 location = {4, 5};
  char mapCharacter = 'A';
  uint32_t zIdx = 1;

  ActionBehaviourDefinition mockBehaviourDefinition;

  mockBehaviourDefinition.behaviourType = ActionBehaviourType::SOURCE;
  mockBehaviourDefinition.sourceObjectName = objectAName;
  mockBehaviourDefinition.destinationObjectName = objectBName;
  mockBehaviourDefinition.actionName = "actionA";
  mockBehaviourDefinition.commandName = "eq";
  mockBehaviourDefinition.commandArguments = {{"0", _Y("0")}, {"1", _Y("10")}};
  mockBehaviourDefinition.conditionalCommands = {{"reward", {{"0", _Y("1")}}}};
  mockBehaviourDefinition.executionProbability = 0.4;

  auto mockObjectPtr = mockObject(objectAName, mapCharacter, 1, zIdx, location, orientation, {"actionA"}, {{"variable1", _V(10)}, {"variable2", _V(20)}});

  EXPECT_CALL(*mockObjectPtr, getRenderTileId).WillOnce(Return(4));

  auto mockGridPtr = std::make_shared<MockGrid>();

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables = {
      {"globalVariable1", {{0, _V(1)}}},
      {"globalVariable2", {{0, _V(1)}, {1, _V(2)}}},
  };

  EXPECT_CALL(*mockGridPtr, getGlobalVariables()).WillOnce(ReturnRef(globalVariables));

  auto objectGenerator = std::make_shared<ObjectGenerator>();

  objectGenerator->setAvatarObject(objectAName);
  objectGenerator->defineNewObject(objectAName, mapCharacter, zIdx, {{"variable1", 0}, {"variable2", 0}});
  objectGenerator->defineActionBehaviour(objectAName, mockBehaviourDefinition);

  auto clonedObject = objectGenerator->cloneInstance(mockObjectPtr, mockGridPtr);

  ASSERT_EQ(clonedObject->getObjectName(), objectAName);
  // ASSERT_EQ(clonedObject->getLocation(), location); // At this point the object has not been added to the grid, so it does not have a location or orientation
  // ASSERT_EQ(clonedObject->getObjectOrientation().getDirection(), orientation.getDirection());
  ASSERT_EQ(clonedObject->getZIdx(), zIdx);
  ASSERT_EQ(clonedObject->getMapCharacter(), mapCharacter);
  ASSERT_EQ(clonedObject->getObjectRenderTileName(), objectAName + "4");
  ASSERT_EQ(clonedObject->isPlayerAvatar(), true);
  ASSERT_THAT(clonedObject->getAvailableActionNames(), UnorderedElementsAre("actionA"));
}

TEST(ObjectGeneratorTest, toObjectData) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  std::map<std::string, GlobalVariableDefinition> globalVariableDefinitions = {
    {"globalVariable1", {0, false}},
    {"globalVariable2", {0, true}}
  };

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables = {
      {"globalVariable1", {{0, _V(1)}}},
      {"globalVariable2", {{0, _V(1)}, {1, _V(1)}, {2, _V(2)}, {3, _V(3)}, {4, _V(4)}}},
  };

  EXPECT_CALL(*mockGridPtr, getGlobalVariables()).WillRepeatedly(ReturnRef(globalVariables));

  auto objectGenerator = std::make_shared<ObjectGenerator>();
  objectGenerator->defineNewObject("objectA", 'A', 0, {{"variableA1", 10}, {"variableA2", 20}});
  objectGenerator->defineNewObject("objectB", 'B', 0, {{"variableB1", 30}, {"variableB2", 40}});

  objectGenerator->defineGlobalVariables(globalVariableDefinitions);

  auto objectA1 = objectGenerator->newInstance("objectA", 1, mockGridPtr);
  objectA1->init({1,2}, DiscreteOrientation(Direction::RIGHT));
  auto objectA2 = objectGenerator->newInstance("objectA", 2, mockGridPtr);
  objectA2->init({3,4}, DiscreteOrientation(Direction::DOWN));

  auto objectB1 = objectGenerator->newInstance("objectB", 3, mockGridPtr);
  objectB1->init({5,6}, DiscreteOrientation(Direction::LEFT));
  auto objectB2 = objectGenerator->newInstance("objectB", 4, mockGridPtr);
  objectB2->init({7,8}, DiscreteOrientation(Direction::UP));

  const auto& stateMapping = objectGenerator->getStateMapping();

  ASSERT_EQ(stateMapping.objectVariableNameToIdx.at("objectA").size(), 8);
  ASSERT_EQ(stateMapping.objectVariableNameToIdx.at("objectB").size(), 8);

  ASSERT_EQ(stateMapping.globalVariableNameToIdx.at("globalVariable1"), 0);
  ASSERT_EQ(stateMapping.globalVariableNameToIdx.at("globalVariable2"), 1);

  const auto& objectA1Data = objectGenerator->toObjectData(objectA1);
  const auto& objectA2Data = objectGenerator->toObjectData(objectA2);
  const auto& objectB1Data = objectGenerator->toObjectData(objectB1);
  const auto& objectB2Data = objectGenerator->toObjectData(objectB2);

  const auto& objectA1VariableIndexes = objectA1Data.getVariableIndexes(stateMapping);
  ASSERT_EQ(objectA1VariableIndexes.size(), 8);
  ASSERT_EQ(objectA1Data.name, "objectA");
  ASSERT_EQ(objectA1Data.getLocation(objectA1VariableIndexes), glm::ivec2(1, 2));
  ASSERT_EQ(objectA1Data.getOrientation(objectA1VariableIndexes).getDirection(), Direction::RIGHT);
  ASSERT_EQ(objectA1Data.getVariableValue(objectA1VariableIndexes, "_playerId"), 1);
  ASSERT_EQ(objectA1Data.getVariableValue(objectA1VariableIndexes, "_renderTileId"), 0);
  ASSERT_EQ(objectA1Data.getVariableValue(objectA1VariableIndexes, "variableA1"), 10);
  ASSERT_EQ(objectA1Data.getVariableValue(objectA1VariableIndexes, "variableA2"), 20);

  const auto& objectA2VariableIndexes = objectA2Data.getVariableIndexes(stateMapping);
  ASSERT_EQ(objectA2VariableIndexes.size(), 8);
  ASSERT_EQ(objectA2Data.name, "objectA");
  ASSERT_EQ(objectA2Data.getLocation(objectA1VariableIndexes), glm::ivec2(3, 4));
  ASSERT_EQ(objectA2Data.getOrientation(objectA1VariableIndexes).getDirection(), Direction::DOWN);
  ASSERT_EQ(objectA2Data.getVariableValue(objectA2VariableIndexes, "_playerId"), 2);
  ASSERT_EQ(objectA2Data.getVariableValue(objectA2VariableIndexes, "_renderTileId"), 0);
  ASSERT_EQ(objectA2Data.getVariableValue(objectA2VariableIndexes, "variableA1"), 10);
  ASSERT_EQ(objectA2Data.getVariableValue(objectA2VariableIndexes, "variableA2"), 20);

  const auto& objectB1VariableIndexes = objectB1Data.getVariableIndexes(stateMapping);
  ASSERT_EQ(objectB1VariableIndexes.size(), 8);
  ASSERT_EQ(objectB1Data.name, "objectB");
  ASSERT_EQ(objectB1Data.getLocation(objectA1VariableIndexes), glm::ivec2(5, 6));
  ASSERT_EQ(objectB1Data.getOrientation(objectA1VariableIndexes).getDirection(), Direction::LEFT);
  ASSERT_EQ(objectB1Data.getVariableValue(objectB1VariableIndexes, "_playerId"), 3);
  ASSERT_EQ(objectB1Data.getVariableValue(objectB1VariableIndexes, "_renderTileId"), 0);
  ASSERT_EQ(objectB1Data.getVariableValue(objectB1VariableIndexes, "variableB1"), 30);
  ASSERT_EQ(objectB1Data.getVariableValue(objectB1VariableIndexes, "variableB2"), 40);

  const auto& objectB2VariableIndexes = objectB2Data.getVariableIndexes(stateMapping);
  ASSERT_EQ(objectB2VariableIndexes.size(), 8);
  ASSERT_EQ(objectB2Data.name, "objectB");
  ASSERT_EQ(objectB2Data.getLocation(objectA1VariableIndexes), glm::ivec2(7, 8));
  ASSERT_EQ(objectB2Data.getOrientation(objectA1VariableIndexes).getDirection(), Direction::UP);
  ASSERT_EQ(objectB2Data.getVariableValue(objectB2VariableIndexes, "_playerId"), 4);
  ASSERT_EQ(objectB2Data.getVariableValue(objectB2VariableIndexes, "_renderTileId"), 0);
  ASSERT_EQ(objectB2Data.getVariableValue(objectB2VariableIndexes, "variableB1"), 30);
  ASSERT_EQ(objectB2Data.getVariableValue(objectB2VariableIndexes, "variableB2"), 40);
}

TEST(ObjectGeneratorTest, fromObjectData) {
}

}  // namespace griddly