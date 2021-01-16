#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>

#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Mocks//Griddly/Core/LevelGenerators/MockLevelGenerator.hpp"
#include "Mocks/Griddly/Core/GDY/MockTerminationGenerator.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define _Y(X) YAML::Node(X)

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

YAML::Node loadFromStringAndGetNode(std::string yamlString, std::string nodeName) {
  auto node = YAML::Load(yamlString.c_str());
  return node[nodeName];
}

TEST(GDYFactoryTest, loadEnvironment) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironment.yaml", "Environment");

  std::string objectName = "object";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar)
      .WillRepeatedly(ReturnRef(objectName));

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(std::vector<std::string>{"base:count", "0"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::WIN), Eq("eq"), Eq(std::vector<std::string>{"_score", "10"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::NONE), Eq("eq"), Eq(std::vector<std::string>{"_steps", "100"})))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getNumLevels(), 1);

  auto globalVariableDefinitions = gdyFactory->getGlobalVariableDefinitions();
  ASSERT_EQ(globalVariableDefinitions["global_variable1"].initialValue, 50);
  ASSERT_EQ(globalVariableDefinitions["global_variable1"].perPlayer, false);
  ASSERT_EQ(globalVariableDefinitions["global_variable2"].initialValue, 0);
  ASSERT_EQ(globalVariableDefinitions["global_variable2"].perPlayer, true);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_Observer) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironmentObserver.yaml", "Environment");

  EXPECT_CALL(*mockObjectGeneratorPtr, setAvatarObject(Eq("avatar")))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getNumLevels(), 0);

  auto observationDefinition = gdyFactory->getPlayerObserverDefinition();

  ASSERT_EQ(observationDefinition.gridHeight, 1);
  ASSERT_EQ(observationDefinition.gridWidth, 2);
  ASSERT_EQ(observationDefinition.gridXOffset, 3);
  ASSERT_EQ(observationDefinition.gridYOffset, 4);
  ASSERT_TRUE(observationDefinition.trackAvatar);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_BlockObserverConfig) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Block2D:
      TileSize: 24
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getNumLevels(), 0);

  auto config = gdyFactory->getBlockObserverConfig();

  ASSERT_EQ(config.tileSize, glm::ivec2(24, 24));
  ASSERT_EQ(config.isoTileDepth, 0);
  ASSERT_EQ(config.isoTileHeight, 0);
  ASSERT_EQ(config.gridXOffset, 0);
  ASSERT_EQ(config.gridYOffset, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_SpriteObserverConfig) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Sprite2D:
      TileSize: 24
      BackgroundTile: oryx/oryx_fantasy/floor2-2.png
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getNumLevels(), 0);

  auto config = gdyFactory->getSpriteObserverConfig();

  ASSERT_EQ(config.tileSize, glm::ivec2(24, 24));
  ASSERT_EQ(config.isoTileDepth, 0);
  ASSERT_EQ(config.isoTileHeight, 0);
  ASSERT_EQ(config.gridXOffset, 0);
  ASSERT_EQ(config.gridYOffset, 0);

  auto spriteDefinitions = gdyFactory->getSpriteObserverDefinitions();
  auto backgroundTile = spriteDefinitions["_background_"];
  ASSERT_EQ(backgroundTile.images[0], "oryx/oryx_fantasy/floor2-2.png");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_IsometricSpriteObserverConfig) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Isometric:
      TileSize: [32, 48]
      IsoTileDepth: 4
      IsoTileHeight: 16
      BackgroundTile: oryx/oryx_iso_dungeon/grass.png
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getNumLevels(), 0);

  auto config = gdyFactory->getIsometricSpriteObserverConfig();

  ASSERT_EQ(config.tileSize, glm::ivec2(32, 48));
  ASSERT_EQ(config.isoTileDepth, 4);
  ASSERT_EQ(config.isoTileHeight, 16);
  ASSERT_EQ(config.gridXOffset, 0);
  ASSERT_EQ(config.gridYOffset, 0);

  auto spriteDefinitions = gdyFactory->getIsometricSpriteObserverDefinitions();
  auto backgroundTile = spriteDefinitions["_iso_background_"];
  ASSERT_EQ(backgroundTile.images[0], "oryx/oryx_iso_dungeon/grass.png");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_ObserverNoAvatar) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironmentObserverNoAvatar.yaml", "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getNumLevels(), 0);

  auto observationDefinition = gdyFactory->getPlayerObserverDefinition();

  // the AvatarObject: avatarName is missing so we default to selective control + no avatar tracking
  ASSERT_EQ(observationDefinition.gridHeight, 0);
  ASSERT_EQ(observationDefinition.gridWidth, 0);
  ASSERT_EQ(observationDefinition.gridXOffset, 0);
  ASSERT_EQ(observationDefinition.gridYOffset, 0);
  ASSERT_FALSE(observationDefinition.trackAvatar);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadObjects) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto objectsNode = loadAndGetNode("tests/resources/loadObjects.yaml", "Objects");

  auto expectedVariables = std::unordered_map<std::string, uint32_t>{{"resources", 0}, {"health", 10}};

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq(0), Eq('O'), Eq(expectedVariables)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple_sprite"), Eq(0), Eq('M'), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple"), Eq(0), Eq(0), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  auto blockObserverDefinitions = gdyFactory->getBlockObserverDefinitions();
  auto spriteObserverDefinitions = gdyFactory->getSpriteObserverDefinitions();

  ASSERT_EQ(1, blockObserverDefinitions.size());
  ASSERT_EQ(3, spriteObserverDefinitions.size());

  // block observer definitions
  auto blockObserverDefinition = blockObserverDefinitions["object0"];
  ASSERT_EQ(blockObserverDefinition.shape, "triangle");
  ASSERT_THAT(blockObserverDefinition.color, ElementsAreArray({0.0, 1.0, 0.0}));
  ASSERT_EQ(blockObserverDefinition.scale, 1.0);

  // sprite observer definitions
  auto spriteObserverDefinition1 = spriteObserverDefinitions["object0"];
  ASSERT_EQ(spriteObserverDefinition1.images, std::vector<std::string>{"object0.png"});
  ASSERT_EQ(spriteObserverDefinition1.tilingMode, TilingMode::NONE);

  auto spriteObserverDefinition2 = spriteObserverDefinitions["object1"];
  ASSERT_EQ(spriteObserverDefinition2.images, std::vector<std::string>{"object1.png"});
  ASSERT_EQ(spriteObserverDefinition2.tilingMode, TilingMode::NONE);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, load_object_initial_actions) {
  auto yamlString = R"(
Objects:
  - Name: object
    InitialActions:
      - Action: action_1
        Delay: 10
        ActionId: 2
      - Action: action_2
        Randomize: true

)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto objectsNode = loadFromStringAndGetNode(std::string(yamlString), "Objects");

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq(0), Eq('\0'), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, addInitialAction(Eq("object"), Eq("action_1"), Eq(2), Eq(10), Eq(false)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, addInitialAction(Eq("object"), Eq("action_2"), Eq(0), Eq(0), Eq(true)))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

bool commandArgumentsEqual(BehaviourCommandArguments a, BehaviourCommandArguments b) {
  for (auto it = a.begin(); it != a.end(); ++it) {
    auto key = it->first;
    auto node = it->second;

    if (node.Type() != b[key].Type()) {
      return false;
    }
  }
  return true;
}

//! the comparison here is not comparing the values of the YAML but just the types. Its not perfect.
MATCHER_P(ActionBehaviourDefinitionEqMatcher, behaviour, "") {
  auto isEqual = behaviour.behaviourType == arg.behaviourType &&
                 behaviour.sourceObjectName == arg.sourceObjectName &&
                 behaviour.destinationObjectName == arg.destinationObjectName &&
                 behaviour.actionName == arg.actionName &&
                 behaviour.commandName == arg.commandName &&
                 commandArgumentsEqual(behaviour.commandArguments, arg.commandArguments);
  //behaviour.actionPreconditions == arg.actionPreconditions &&
  //behaviour.conditionalCommands == arg.conditionalCommands;

  return isEqual;
}

void expectOpposingDefinitionNOP(ActionBehaviourType behaviourType, std::shared_ptr<MockObjectGenerator> mockObjectGeneratorPtr) {
  ActionBehaviourDefinition expectedNOPDefinition = GDYFactory::makeBehaviourDefinition(
      behaviourType == ActionBehaviourType::DESTINATION ? ActionBehaviourType::SOURCE : ActionBehaviourType::DESTINATION,
      behaviourType == ActionBehaviourType::DESTINATION ? "sourceObject" : "destinationObject",
      behaviourType == ActionBehaviourType::SOURCE ? "sourceObject" : "destinationObject",
      "action",
      "nop",
      {},
      {},
      {});

  auto objectName = behaviourType == ActionBehaviourType::SOURCE ? "destinationObject" : "sourceObject";

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq(objectName), ActionBehaviourDefinitionEqMatcher(expectedNOPDefinition)))
      .Times(1);
}

void testBehaviourDefinition(std::string yamlString, ActionBehaviourDefinition expectedBehaviourDefinition, bool expectNOP) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  auto objectName = expectedBehaviourDefinition.behaviourType == ActionBehaviourType::SOURCE ? "sourceObject" : "destinationObject";

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq(objectName), ActionBehaviourDefinitionEqMatcher(expectedBehaviourDefinition)))
      .Times(1);

  if (expectNOP) {
    expectOpposingDefinitionNOP(expectedBehaviourDefinition.behaviourType, mockObjectGeneratorPtr);
  }

  gdyFactory->loadActions(actionsNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadAction_source_precondition) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: sourceObject
          Preconditions:
            - eq: ["counter", 5]
          Commands:
            - incr: resources
        Dst:
          Object: destinationObject
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "incr",
      {{"0", _Y("resources")}},
      {{{"eq", {{"0", _Y("counter")}, {"1", _Y("5")}}}}},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_source_conditional) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: sourceObject
          Commands: 
            - eq:
                Arguments: [0, 1]
                Commands:
                  - reward: 1
        Dst:
          Object: destinationObject
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "eq",
      {{"0", _Y("0")}, {"1", _Y("1")}},
      {},
      {{"reward", {{"0", _Y("1")}}}});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_source_named_arguments) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: sourceObject
          Commands:
            - exec:
                Action: other
                VectorToDest: _dest
                Delay: 10
        Dst:
          Object: destinationObject
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "sourceObject",
      "destinationObject",
      "action",
      "exec",
      {{"Action", _Y("other")}, {"Delay", _Y("10")}, {"VectorToDest", _Y("_dest")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_destination) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: sourceObject
        Dst:
          Object: destinationObject
          Commands:
            - decr: resources
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "decr",
      {{"0", _Y("resources")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_destination_conditional) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: sourceObject
        Dst:
          Object: destinationObject
          Commands:
            - eq:
                Arguments: [0, 1]
                Commands:
                  - multi: [0, 1, 2]
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "destinationObject",
      "sourceObject",
      "action",
      "eq",
      {{"0", _Y("0")}, {"1", _Y("1")}},
      {},
      {{"multi", {{"0", _Y("0")}, {"1", _Y("1")}, {"2", _Y("2")}}}});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefs(std::vector<std::string> objectNames) {
  std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions;
  for (auto name : objectNames) {
    ObjectDefinition objectDefinition = {
        name};
    mockObjectDefinitions[name] = std::make_shared<ObjectDefinition>(objectDefinition);
  }

  return mockObjectDefinitions;
}

TEST(GDYFactoryTest, wallTest) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto grid = std::shared_ptr<Grid>(new Grid());

  auto mockWall2Object = std::shared_ptr<MockObject>(new MockObject());
  auto mockWall16Object = std::shared_ptr<MockObject>(new MockObject());

  std::string wall2String = "Wall2";
  std::string wall16String = "Wall16";

  EXPECT_CALL(*mockWall2Object, getObjectName())
      .WillRepeatedly(Return(wall2String));

  EXPECT_CALL(*mockWall16Object, getObjectName())
      .WillRepeatedly(Return(wall16String));

  auto objectDefinitions = mockObjectDefs({wall2String, wall16String});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall2String));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .WillRepeatedly(ReturnRef(wall16String));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall2String), Eq(0), _))
      .WillRepeatedly(Return(mockWall2Object));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall16String), Eq(0), _))
      .WillRepeatedly(Return(mockWall16Object));

  gdyFactory->initializeFromFile("tests/resources/walls.yaml");
  gdyFactory->getLevelGenerator(0)->reset(grid);

  ASSERT_EQ(grid->getWidth(), 17);
  ASSERT_EQ(grid->getHeight(), 17);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, zIndexTest) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto grid = std::shared_ptr<Grid>(new Grid());

  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockFloorObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGhostObject = std::shared_ptr<MockObject>(new MockObject());
  std::string wall = "Wall2";
  std::string floor = "floor";
  std::string ghost = "ghost";

  EXPECT_CALL(*mockWallObject, getObjectName())
      .WillRepeatedly(Return(wall));

  EXPECT_CALL(*mockFloorObject, getObjectName())
      .WillRepeatedly(Return(floor));

  EXPECT_CALL(*mockGhostObject, getObjectName())
      .WillRepeatedly(Return(ghost));

  auto objectDefinitions = mockObjectDefs({wall, floor, ghost});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('f')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('g')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall), Eq(0), _))
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(floor), Eq(0), _))
      .WillRepeatedly(Return(mockFloorObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(ghost), Eq(0), _))
      .WillRepeatedly(Return(mockGhostObject));

  gdyFactory->initializeFromFile("tests/resources/ztest.yaml");
  gdyFactory->getLevelGenerator(0)->reset(grid);

  ASSERT_EQ(grid->getWidth(), 5);
  ASSERT_EQ(grid->getHeight(), 5);
}

MATCHER_P(InputMappingMatcherEq, expectedActionInputsDefinitions, "") {
  if (expectedActionInputsDefinitions.size() != arg.size()) {
    return false;
  }

  for (auto expectedActionInputsDefinitionPair : expectedActionInputsDefinitions) {
    auto key = expectedActionInputsDefinitionPair.first;
    auto expectedActionInputsDefinition = expectedActionInputsDefinitionPair.second;
    auto actualActionInputsDefinitionIt = arg.find(key);

    if (actualActionInputsDefinitionIt == arg.end()) {
      return false;
    }

    auto actualActionInputsDefinition = actualActionInputsDefinitionIt->second;

    if (actualActionInputsDefinition.relative != expectedActionInputsDefinition.relative) {
      return false;
    }

    if (actualActionInputsDefinition.internal != expectedActionInputsDefinition.internal) {
      return false;
    }

    if (actualActionInputsDefinition.mapToGrid != expectedActionInputsDefinition.mapToGrid) {
      return false;
    }

    auto actualInputMappings = actualActionInputsDefinition.inputMappings;

    for (auto expectedInputMappingPair : actualActionInputsDefinition.inputMappings) {
      auto actionId = expectedInputMappingPair.first;
      auto expectedInputMapping = expectedInputMappingPair.second;
      auto actualInputMappingIt = actualInputMappings.find(actionId);

      if (actualInputMappingIt == actualInputMappings.end()) {
        return false;
      }
      auto actualInputMapping = actualInputMappingIt->second;

      if (expectedInputMapping.vectorToDest != actualInputMapping.vectorToDest) {
        return false;
      }

      if (expectedInputMapping.orientationVector != actualInputMapping.orientationVector) {
        return false;
      }

      if (expectedInputMapping.description != actualInputMapping.description) {
        return false;
      }

      
    }
  }

  return true;
}

TEST(GDYFactoryTest, action_input_mapping) {
  auto yamlString = R"(
Actions:
  - Name: move
    InputMapping:
      Inputs:
        1: 
          OrientationVector: [1, 0]
          VectorToDest: [1, 0] 
        2:
          OrientationVector: [0, -1]
          VectorToDest: [0, -1]
        3:
          OrientationVector: [-1, 0]
          VectorToDest: [-1, 0] 
        4:
          OrientationVector: [0, 1]
          VectorToDest: [0, 1]   
      Relative: true
)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"move", {{
                    {1, {{1, 0}, {1, 0}, ""}},
                    {2, {{0, -1}, {0, -1}, ""}},
                    {3, {{-1, 0}, {-1, 0}, ""}},
                    {4, {{0, 1}, {0, 1}, ""}},
                },
                true,
                false}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}

TEST(GDYFactoryTest, action_input_default_values) {
  auto yamlString = R"(
Actions:
  - Name: move
    InputMapping:
      Inputs:
        1: 
          Description: Do Something
          OrientationVector: [1, 0]
        2:
          VectorToDest: [0, -1]
        4:
          OrientationVector: [0, 1]
          VectorToDest: [0, 1]   
      Relative: true
)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"move", {{
                    {1, {{0, 0}, {1, 0}, "Do Something"}},
                    {2, {{0, -1}, {0, 0}, ""}},
                    {4, {{0, 1}, {0, 1}, ""}},
                },
                true,
                false}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}

TEST(GDYFactoryTest, action_input_map_to_grid) {
  auto yamlString = R"(
Actions:
  - Name: spawn
    InputMapping:
      MapToGrid: true
)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"spawn", {{},
                false,
                false,
                true}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}

TEST(GDYFactoryTest, action_input_default_mapping) {
  auto yamlString = R"(
Actions:
  - Name: move
)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"move", {{
                    {1, {{-1, 0}, {-1, 0}, "Left"}},
                    {2, {{0, -1}, {0, -1}, "Up"}},
                    {3, {{1, 0}, {1, 0}, "Right"}},
                    {4, {{0, 1}, {0, 1}, "Down"}},
                },
                false,
                false}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}

TEST(GDYFactoryTest, action_input_internal_mapping) {
  auto yamlString = R"(
Actions:
  - Name: player_move
  - Name: other_move
  - Name: internal_move
    InputMapping:
      Internal: true
)";

  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::shared_ptr<MockTerminationGenerator>(new MockTerminationGenerator());
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"player_move",
       {{
            {1, {{-1, 0}, {-1, 0}, "Left"}},
            {2, {{0, -1}, {0, -1}, "Up"}},
            {3, {{1, 0}, {1, 0}, "Right"}},
            {4, {{0, 1}, {0, 1}, "Down"}},
        },
        false,
        false}},
      {"other_move",
       {{
            {1, {{-1, 0}, {-1, 0}, "Left"}},
            {2, {{0, -1}, {0, -1}, "Up"}},
            {3, {{1, 0}, {1, 0}, "Right"}},
            {4, {{0, 1}, {0, 1}, "Down"}},
        },
        false,
        false}},
      {"internal_move",
       {{
            {1, {{-1, 0}, {-1, 0}, "Left"}},
            {2, {{0, -1}, {0, -1}, "Up"}},
            {3, {{1, 0}, {1, 0}, "Right"}},
            {4, {{0, 1}, {0, 1}, "Down"}},
        },
        false,
        true}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}  // namespace griddly

}  // namespace griddly