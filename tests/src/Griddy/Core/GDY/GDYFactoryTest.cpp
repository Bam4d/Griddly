#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>

#include "Griddy/Core/GDY/Actions/Action.hpp"
#include "Griddy/Core/GDY/GDYFactory.hpp"
#include "Mocks//Griddy/Core/LevelGenerators/MockMapReader.cpp"
#include "Mocks/Griddy/Core/GDY/Objects/MockObjectGenerator.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

YAML::Node loadAndGetNode(std::string filename, std::string nodeName) {
  auto node = YAML::LoadFile(filename);
  return node[nodeName];
}

TEST(GDYFactoryTest, createLevel) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr));
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  EXPECT_CALL(*mockGridPtr, init(Eq(10), Eq(12)))
      .Times(1);

  gdyFactory->createLevel(10, 12, mockGridPtr);

}

TEST(GDYFactoryTest, loadEnvironment) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr));
  auto environmentNode = loadAndGetNode("tests/src/Griddy/Core/GDY/resources/loadEnvironment.yaml", "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getNumLevels(), 1);
  ASSERT_EQ(gdyFactory->getTileSize(), 16);
}

TEST(GDYFactoryTest, loadObjects) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr));
  auto objectsNode = loadAndGetNode("tests/src/Griddy/Core/GDY/resources/loadObjects.yaml", "Objects");

  auto expectedParameters = std::unordered_map<std::string, uint32_t>{{"resources", 0}, {"health", 10}};

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq('O'), Eq(expectedParameters)))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  auto blockObserverDefinitions = gdyFactory->getBlockObserverDefinitions();
  auto spriteObserverDefinitions = gdyFactory->getSpriteObserverDefinitions();

  ASSERT_EQ(1, blockObserverDefinitions.size());
  ASSERT_EQ(1, spriteObserverDefinitions.size());

  // block observer definitions
  auto blockObserverDefinition = blockObserverDefinitions["object"];
  ASSERT_EQ(blockObserverDefinition.shape, "triangle");
  ASSERT_THAT(blockObserverDefinition.color, ElementsAreArray({0.0, 1.0, 0.0}));
  ASSERT_EQ(blockObserverDefinition.scale, 1.0);

  // sprite observer definitions
  auto spriteObserverDefinition = spriteObserverDefinitions["object"];
  ASSERT_EQ(spriteObserverDefinition, "object.png");
}

MATCHER_P(ActionBehaviourDefinitionEqMatcher, behaviour, "") {
  auto isEqual = behaviour.behaviourType == arg.behaviourType &&
                 behaviour.sourceObjectName == arg.sourceObjectName &&
                 behaviour.destinationObjectName == arg.destinationObjectName &&
                 behaviour.actionName == arg.actionName &&
                 behaviour.commandName == arg.commandName &&
                 behaviour.commandParameters == arg.commandParameters &&
                 behaviour.conditionalCommands == arg.conditionalCommands;

  return isEqual;
}

TEST(GDYFactoryTest, loadActions) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr));
  auto actionsNode = loadAndGetNode("tests/src/Griddy/Core/GDY/resources/loadActions.yaml", "Actions");

  ActionBehaviourDefinition sourceResourceBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "incr",
      {"resources"},
      {});

  ActionBehaviourDefinition sourceRewardBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "eq",
      {"0", "1"},
      {{"reward", {"1"}}});

  ActionBehaviourDefinition destinationResourceBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "decr",
      {"resources"},
      {});

  ActionBehaviourDefinition destinationMultiBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "multi",
      {"0","1","2"},
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
}
}  // namespace griddy