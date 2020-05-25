#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>

#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Mocks//Griddly/Core/LevelGenerators/MockMapReader.cpp"
#include "Mocks/Griddly/Core/GDY/MockTerminationGenerator.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace griddly {

YAML::Node loadAndGetNode(std::string filename, std::string nodeName) {
  auto node = YAML::LoadFile(filename);
  return node[nodeName];
}

TEST(GDYFactoryTest, createLevel) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(10), Eq(12)))
      .Times(1);

  gdyFactory->createLevel(10, 12, mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironment.yaml", "Environment");

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(std::vector<std::string>{"base:count", "0"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::WIN), Eq("eq"), Eq(std::vector<std::string>{"_score", "10"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::NONE), Eq("eq"), Eq(std::vector<std::string>{"_max_steps", "100"})))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getNumLevels(), 1);
  ASSERT_EQ(gdyFactory->getTileSize(), 16);
  ASSERT_THAT(gdyFactory->getGlobalParameterDefinitions(), UnorderedElementsAre(Pair("global_parameter1", 50), Pair("global_parameter2", 0)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadObjects) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto objectsNode = loadAndGetNode("tests/resources/loadObjects.yaml", "Objects");

  auto expectedParameters = std::unordered_map<std::string, uint32_t>{{"resources", 0}, {"health", 10}};

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq(0), Eq('O'), Eq(expectedParameters)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple_sprite"), Eq(0), Eq('M'), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple"), Eq(0), Eq(0), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  auto blockObserverDefinitions = gdyFactory->getBlockObserverDefinitions();
  auto spriteObserverDefinitions = gdyFactory->getSpriteObserverDefinitions();

  ASSERT_EQ(1, blockObserverDefinitions.size());
  ASSERT_EQ(2, spriteObserverDefinitions.size());

  // block observer definitions
  auto blockObserverDefinition = blockObserverDefinitions["object"];
  ASSERT_EQ(blockObserverDefinition.shape, "triangle");
  ASSERT_THAT(blockObserverDefinition.color, ElementsAreArray({0.0, 1.0, 0.0}));
  ASSERT_EQ(blockObserverDefinition.scale, 1.0);

  // sprite observer definitions
  auto spriteObserverDefinition = spriteObserverDefinitions["object"];
  ASSERT_EQ(spriteObserverDefinition.images, std::vector<std::string>{"object.png"});
  ASSERT_EQ(spriteObserverDefinition.tilingMode, TilingMode::NONE);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

MATCHER_P(ActionBehaviourDefinitionEqMatcher, behaviour, "") {
  auto isEqual = behaviour.behaviourType == arg.behaviourType &&
                 behaviour.sourceObjectName == arg.sourceObjectName &&
                 behaviour.destinationObjectName == arg.destinationObjectName &&
                 behaviour.actionName == arg.actionName &&
                 behaviour.commandName == arg.commandName &&
                 behaviour.commandParameters == arg.commandParameters &&
                 behaviour.actionPreconditions == arg.actionPreconditions &&
                 behaviour.conditionalCommands == arg.conditionalCommands;

  return isEqual;
}

TEST(GDYFactoryTest, loadActions) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto actionsNode = loadAndGetNode("tests/resources/loadActions.yaml", "Actions");

  ActionBehaviourDefinition sourceResourceBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "incr",
      {"resources"},
      {{{"eq", {"counter", "5"}}}},
      {});

  ActionBehaviourDefinition sourceRewardBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "eq",
      {"0", "1"},
      {{{"eq", {"counter", "5"}}}},
      {{"reward", {"1"}}});

  ActionBehaviourDefinition destinationResourceBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "decr",
      {"resources"},
      {},
      {});

  ActionBehaviourDefinition destinationMultiBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "multi",
      {"0", "1", "2"},
      {},
      {});

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq("sourceObject"), ActionBehaviourDefinitionEqMatcher(sourceResourceBehaviourDefinition)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq("sourceObject"), ActionBehaviourDefinitionEqMatcher(sourceRewardBehaviourDefinition)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq("destinationObject"), ActionBehaviourDefinitionEqMatcher(destinationResourceBehaviourDefinition)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq("destinationObject"), ActionBehaviourDefinitionEqMatcher(destinationMultiBehaviourDefinition)))
      .Times(1);

  gdyFactory->loadActions(actionsNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, wallTest) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto grid = std::shared_ptr<Grid>(new Grid());

  auto mockWall2Object = std::shared_ptr<MockObject>(new MockObject());
  auto mockWall16Object = std::shared_ptr<MockObject>(new MockObject());

  std::string wall2String = "Wall2";
  std::string wall16String = "Wall16";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall2String));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .WillRepeatedly(ReturnRef(wall16String));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall2String), _))
      .WillRepeatedly(Return(mockWall2Object));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall16String), _))
      .WillRepeatedly(Return(mockWall16Object));

  gdyFactory->initializeFromFile("tests/resources/walls.yaml");
  gdyFactory->loadLevel(0);
  gdyFactory->getLevelGenerator()->reset(grid);

  ASSERT_EQ(grid->getWidth(), 17);
  ASSERT_EQ(grid->getHeight(), 17);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, zIndexTest) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr));
  auto grid = std::shared_ptr<Grid>(new Grid());

  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockFloorObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGhostObject = std::shared_ptr<MockObject>(new MockObject());
  std::string wall = "Wall2";
  std::string floor = "floor";
  std::string ghost = "ghost";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('f')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('g')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall), _))
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(floor), _))
      .WillRepeatedly(Return(mockFloorObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(ghost), _))
      .WillRepeatedly(Return(mockGhostObject));

  gdyFactory->initializeFromFile("tests/resources/ztest.yaml");
  gdyFactory->loadLevel(0);
  gdyFactory->getLevelGenerator()->reset(grid);

  ASSERT_EQ(grid->getWidth(), 5);
  ASSERT_EQ(grid->getHeight(), 5);
}

}  // namespace griddly