#include <unordered_map>

#include "Griddly/Core/TestUtils/common.hpp"
#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
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
  DiscreteOrientation orientation = DiscreteOrientation(Direction::RIGHT);
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
  mockBehaviourDefinition.actionPreconditions = {{"eq", {{"0", _Y("0")}, {"1", _Y("1")}}}, {"eq", {{"0", _Y("1")}, {"1", _Y("0")}}}};
  mockBehaviourDefinition.conditionalCommands = {{"reward", {{"0", _Y("1")}}}};
  mockBehaviourDefinition.executionProbability = 0.4; 

  auto mockObjectPtr = mockObject(objectAName, mapCharacter, 1, zIdx, location, orientation, {"actionA"}, {{"variable1", _V(10)}, {"variable2", _V(20)}});

  EXPECT_CALL(*mockObjectPtr, getRenderTileId).WillOnce(Return(4));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables = {
    {"globalVariable1", {{0, _V(1)}}},
    {"globalVariable2", {{0, _V(1)},{1, _V(2)}}},
  };

  EXPECT_CALL(*mockGridPtr, getGlobalVariables()).WillOnce(ReturnRef(globalVariables));

  auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());

  objectGenerator->setAvatarObject(objectAName);
  objectGenerator->defineNewObject(objectAName, mapCharacter, zIdx, {{"variable1", 0}, {"variable2", 0}});
  objectGenerator->defineActionBehaviour(objectAName, mockBehaviourDefinition);

  auto clonedObject = objectGenerator->cloneInstance(mockObjectPtr, mockGridPtr);

  ASSERT_EQ(clonedObject->getObjectName(), objectAName);
  //ASSERT_EQ(clonedObject->getLocation(), location); // At this point the object has not been added to the grid, so it does not have a location or orientation
  //ASSERT_EQ(clonedObject->getObjectOrientation().getDirection(), orientation.getDirection());
  ASSERT_EQ(clonedObject->getZIdx(), zIdx);
  ASSERT_EQ(clonedObject->getMapCharacter(), mapCharacter);
  ASSERT_EQ(clonedObject->getObjectRenderTileName(), objectAName+"4");
  ASSERT_EQ(clonedObject->isPlayerAvatar(), true);
  ASSERT_THAT(clonedObject->getAvailableActionNames(), UnorderedElementsAre("actionA"));

}
}  // namespace griddly