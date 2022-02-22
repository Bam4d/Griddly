#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>

#include "Griddly/Core/TestUtils/common.hpp"
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
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironment.yaml", "Environment");

  std::string objectName = "object";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .WillRepeatedly(ReturnRef(objectName));

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getLevelCount(), 1);

  auto globalVariableDefinitions = gdyFactory->getGlobalVariableDefinitions();
  ASSERT_EQ(globalVariableDefinitions["global_variable1"].initialValue, 50);
  ASSERT_EQ(globalVariableDefinitions["global_variable1"].perPlayer, false);
  ASSERT_EQ(globalVariableDefinitions["global_variable2"].initialValue, 0);
  ASSERT_EQ(globalVariableDefinitions["global_variable2"].perPlayer, true);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_VectorObserverConfig_playerId) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Vector:
      IncludePlayerId: True
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VectorObserverConfig>(gdyFactory->getNamedObserverConfig("VECTOR"));

  ASSERT_EQ(config.includePlayerId, true);
  ASSERT_EQ(config.includeRotation, false);
  ASSERT_EQ(config.includeVariables, false);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_VectorObserverConfig_variables) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Vector:
      IncludeVariables: True
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VectorObserverConfig>(gdyFactory->getNamedObserverConfig("VECTOR"));

  ASSERT_EQ(config.includePlayerId, false);
  ASSERT_EQ(config.includeRotation, false);
  ASSERT_EQ(config.includeVariables, true);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_VectorObserverConfig_rotation) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Vector:
      IncludeRotation: True
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VectorObserverConfig>(gdyFactory->getNamedObserverConfig("VECTOR"));

  ASSERT_EQ(config.includePlayerId, false);
  ASSERT_EQ(config.includeRotation, true);
  ASSERT_EQ(config.includeVariables, false);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_VectorObserverConfig_playerId_rotation_variables) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test
  Description: Test Description
  Observers:
    Vector:
      IncludePlayerId: True
      IncludeVariables: True
      IncludeRotation: True
)";

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VectorObserverConfig>(gdyFactory->getNamedObserverConfig("VECTOR"));

  ASSERT_EQ(config.includePlayerId, true);
  ASSERT_EQ(config.includeRotation, true);
  ASSERT_EQ(config.includeVariables, true);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_Observer) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironmentObserver.yaml", "Environment");

  EXPECT_CALL(*mockObjectGeneratorPtr, setAvatarObject(Eq("avatar")))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 1);
  ASSERT_EQ(observerConfig.overrideGridWidth, 2);
  ASSERT_EQ(observerConfig.gridXOffset, 3);
  ASSERT_EQ(observerConfig.gridYOffset, 4);
  ASSERT_TRUE(observerConfig.trackAvatar);


  auto defaultVectorObserverConfig = *gdyFactory->getNamedObserverConfig("VECTOR");
  ASSERT_EQ(defaultVectorObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultVectorObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultVectorObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultVectorObserverConfig.gridYOffset, 4);
  ASSERT_TRUE(defaultVectorObserverConfig.trackAvatar);
  auto defaultBlockObserverConfig = *gdyFactory->getNamedObserverConfig("BLOCK_2D");
  ASSERT_EQ(defaultBlockObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultBlockObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultBlockObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultBlockObserverConfig.gridYOffset, 4);
  ASSERT_TRUE(defaultBlockObserverConfig.trackAvatar);
  auto defaultSpriteObserverConfig = *gdyFactory->getNamedObserverConfig("SPRITE_2D");
  ASSERT_EQ(defaultSpriteObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultSpriteObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultSpriteObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultSpriteObserverConfig.gridYOffset, 4);
  ASSERT_TRUE(defaultSpriteObserverConfig.trackAvatar);
  auto defaultASCIIObserverConfig = *gdyFactory->getNamedObserverConfig("ASCII");
  ASSERT_EQ(defaultASCIIObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultASCIIObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultASCIIObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultASCIIObserverConfig.gridYOffset, 4);
  ASSERT_TRUE(defaultASCIIObserverConfig.trackAvatar);
  auto defaultIsometricObserverConfig = *gdyFactory->getNamedObserverConfig("ISOMETRIC");
  ASSERT_EQ(defaultIsometricObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultIsometricObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultIsometricObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultIsometricObserverConfig.gridYOffset, 4);
  ASSERT_TRUE(defaultIsometricObserverConfig.trackAvatar);



  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_NamedObservers) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
Environment:
  Name: Test Environment
  TileSize: 16
  Observers: 
    TestVectorObserver:
      Type: VECTOR
      TrackAvatar: false
      IncludePlayerId: true
    TestSprite2DObserver:
      Type: SPRITE_2D
      RotateWithAvatar: false
      RotateAvatarImage: true
      Height: 10
      Width: 10
      OffsetX: 0
      OffsetY: 0
  Player:
    Observer:
      TrackAvatar: true
      RotateWithAvatar: true
      RotateAvatarImage: false
      Height: 1
      Width: 2
      OffsetX: 3
      OffsetY: 4
    AvatarObject: avatar # The player can only control a single avatar in the game
)";

  EXPECT_CALL(*mockObjectGeneratorPtr, setAvatarObject(Eq("avatar")))
      .Times(1);

  auto environmentNode = loadFromStringAndGetNode(yamlString, "Environment");

  gdyFactory->loadEnvironment02(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 1);
  ASSERT_EQ(observerConfig.overrideGridWidth, 2);
  ASSERT_EQ(observerConfig.gridXOffset, 3);
  ASSERT_EQ(observerConfig.gridYOffset, 4);
  ASSERT_TRUE(observerConfig.trackAvatar);
  ASSERT_FALSE(observerConfig.rotateAvatarImage);
  ASSERT_TRUE(observerConfig.rotateWithAvatar);


  auto defaultVectorObserverConfig = *std::static_pointer_cast<VectorObserverConfig>(gdyFactory->getNamedObserverConfig("TestVectorObserver"));
  ASSERT_EQ(defaultVectorObserverConfig.overrideGridHeight, 1);
  ASSERT_EQ(defaultVectorObserverConfig.overrideGridWidth, 2);
  ASSERT_EQ(defaultVectorObserverConfig.gridXOffset, 3);
  ASSERT_EQ(defaultVectorObserverConfig.gridYOffset, 4);
  ASSERT_FALSE(defaultVectorObserverConfig.trackAvatar);
  ASSERT_TRUE(defaultVectorObserverConfig.includePlayerId);
  auto defaultBlockObserverConfig = *std::static_pointer_cast<VulkanGridObserverConfig>(gdyFactory->getNamedObserverConfig("TestSprite2DObserver"));
  ASSERT_EQ(defaultBlockObserverConfig.overrideGridHeight, 10);
  ASSERT_EQ(defaultBlockObserverConfig.overrideGridWidth, 10);
  ASSERT_EQ(defaultBlockObserverConfig.gridXOffset, 0);
  ASSERT_EQ(defaultBlockObserverConfig.gridYOffset, 0);
  ASSERT_TRUE(defaultBlockObserverConfig.trackAvatar);
  ASSERT_FALSE(defaultBlockObserverConfig.rotateWithAvatar);
  ASSERT_TRUE(defaultBlockObserverConfig.rotateAvatarImage);
}

TEST(GDYFactoryTest, loadEnvironment_BlockObserverConfig) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VulkanGridObserverConfig>(gdyFactory->getNamedObserverConfig("BLOCK_2D"));

  ASSERT_EQ(config.tileSize, glm::ivec2(24, 24));
  ASSERT_EQ(config.gridXOffset, 0);
  ASSERT_EQ(config.gridYOffset, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_SpriteObserverConfig) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<VulkanGridObserverConfig>(gdyFactory->getNamedObserverConfig("SPRITE_2D"));

  ASSERT_EQ(config.tileSize, glm::ivec2(24, 24));
  ASSERT_EQ(config.gridXOffset, 0);
  ASSERT_EQ(config.gridYOffset, 0);

  auto spriteDefinitions = gdyFactory->getSpriteObserverDefinitions();
  auto backgroundTile = spriteDefinitions["_background_"];
  ASSERT_EQ(backgroundTile.images[0], "oryx/oryx_fantasy/floor2-2.png");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_IsometricSpriteObserverConfig) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto config = *std::static_pointer_cast<IsometricSpriteObserverConfig>(gdyFactory->getNamedObserverConfig("ISOMETRIC"));

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
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto environmentNode = loadAndGetNode("tests/resources/loadEnvironmentObserverNoAvatar.yaml", "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  ASSERT_EQ(gdyFactory->getName(), "Test Environment");
  ASSERT_EQ(gdyFactory->getLevelCount(), 0);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  // the AvatarObject: avatarName is missing so we default to selective control + no avatar tracking
  ASSERT_EQ(observerConfig.overrideGridHeight, 0);
  ASSERT_EQ(observerConfig.overrideGridWidth, 0);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_FALSE(observerConfig.trackAvatar);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_PlayerHighlight) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, nullptr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Player:
      AvatarObject: player
      Observer:
        HighlightPlayers: true
        TrackAvatar: true
        Height: 9
        Width: 9
  )";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 9);
  ASSERT_EQ(observerConfig.overrideGridWidth, 9);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_TRUE(observerConfig.highlightPlayers);
  ASSERT_TRUE(observerConfig.trackAvatar);
}

TEST(GDYFactoryTest, loadEnvironment_PlayerNoHighlight) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, nullptr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Player:
      AvatarObject: player
      Observer:
        TrackAvatar: true
        Height: 9
        Width: 9
  )";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 9);
  ASSERT_EQ(observerConfig.overrideGridWidth, 9);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_FALSE(observerConfig.highlightPlayers);
  ASSERT_TRUE(observerConfig.trackAvatar);
  ASSERT_FALSE(observerConfig.rotateWithAvatar);
  ASSERT_TRUE(observerConfig.rotateAvatarImage);
}

TEST(GDYFactoryTest, loadEnvironment_RotateAvatar) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, nullptr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Player:
      AvatarObject: player
      Observer:
        RotateAvatarImage: false
        Height: 9
        Width: 9
  )";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 9);
  ASSERT_EQ(observerConfig.overrideGridWidth, 9);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_FALSE(observerConfig.highlightPlayers);
  ASSERT_FALSE(observerConfig.trackAvatar);
  ASSERT_FALSE(observerConfig.rotateWithAvatar);
  ASSERT_FALSE(observerConfig.rotateAvatarImage);
}

TEST(GDYFactoryTest, loadEnvironment_MultiPlayerNoHighlight) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, nullptr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Player:
      AvatarObject: player
      Count: 10
      Observer:
        HighlightPlayers: false
        TrackAvatar: true
        Height: 9
        Width: 9
  )";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 9);
  ASSERT_EQ(observerConfig.overrideGridWidth, 9);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_FALSE(observerConfig.highlightPlayers);
  ASSERT_TRUE(observerConfig.trackAvatar);
}

TEST(GDYFactoryTest, loadEnvironment_MultiPlayerHighlight) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, nullptr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Player:
      AvatarObject: player
      Count: 10
      Observer:
        TrackAvatar: true
        Height: 9
        Width: 9
  )";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  gdyFactory->loadEnvironment(environmentNode);

  auto observerConfig = gdyFactory->getDefaultObserverConfig();

  ASSERT_EQ(observerConfig.overrideGridHeight, 9);
  ASSERT_EQ(observerConfig.overrideGridWidth, 9);
  ASSERT_EQ(observerConfig.gridXOffset, 0);
  ASSERT_EQ(observerConfig.gridYOffset, 0);
  ASSERT_TRUE(observerConfig.highlightPlayers);
  ASSERT_TRUE(observerConfig.trackAvatar);
}

TEST(GDYFactoryTest, loadEnvironment_termination_v1) {
  //auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(nullptr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
  Environment:
    Name: Test
    Termination:
      Lose:
        - eq: [var1, -10]
        - eq: [var2, -10]
      Win: 
        - gt: [var2, 10]
      End: 
        - lt: [var3, -1]
)";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var1", "-10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var2", "-10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::WIN), Eq("gt"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var2", "10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::NONE), Eq("lt"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var3", "-1"})))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadEnvironment_termination_v2) {
  //auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(nullptr, mockTerminationGeneratorPtr, {}));
  auto yamlString = R"(
    Environment:
      Name: Test
      Termination:
        Lose:
          - Conditions:
              - eq: [var1, -10]
            Reward: -5
            OpposingReward: 5
          - Conditions:
              - eq: [var2, -10]
            Reward: -15
            OpposingReward: 15
        Win: 
          - Conditions:
              - gte: [var2, -10]
            Reward: 15
            OpposingReward: -15
          - Conditions:
              - gt: [var2, 10]
        End: 
          - Conditions:
              - eq: [var1, -10]
            Reward: -5
            OpposingReward: 5
          - Conditions:
              - lt: [var3, -1]
)";

  auto environmentNode = loadFromStringAndGetNode(std::string(yamlString), "Environment");

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(-5), Eq(5), Eq(std::vector<std::string>{"var1", "-10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::LOSE), Eq("eq"), Eq(-15), Eq(15), Eq(std::vector<std::string>{"var2", "-10"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::WIN), Eq("gte"), Eq(15), Eq(-15), Eq(std::vector<std::string>{"var2", "-10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::WIN), Eq("gt"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var2", "10"})))
      .Times(1);

  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::NONE), Eq("eq"), Eq(-5), Eq(5), Eq(std::vector<std::string>{"var1", "-10"})))
      .Times(1);
  EXPECT_CALL(*mockTerminationGeneratorPtr, defineTerminationCondition(Eq(TerminationState::NONE), Eq("lt"), Eq(0), Eq(0), Eq(std::vector<std::string>{"var3", "-1"})))
      .Times(1);

  gdyFactory->loadEnvironment(environmentNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationGeneratorPtr.get()));
}

TEST(GDYFactoryTest, loadObjects) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto objectsNode = loadAndGetNode("tests/resources/loadObjects.yaml", "Objects");

  auto expectedVariables = std::unordered_map<std::string, uint32_t>{{"resources", 0}, {"health", 10}};

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq('O'), Eq(0), Eq(expectedVariables)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple_sprite"), Eq('M'), Eq(0), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object_simple"), Eq('?'), Eq(0), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  auto blockObserverDefinitions = gdyFactory->getBlockObserverDefinitions();
  auto spriteObserverDefinitions = gdyFactory->getSpriteObserverDefinitions();

  ASSERT_EQ(3, blockObserverDefinitions.size());
  ASSERT_EQ(4, spriteObserverDefinitions.size());

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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto objectsNode = loadFromStringAndGetNode(std::string(yamlString), "Objects");

  EXPECT_CALL(*mockObjectGeneratorPtr, defineNewObject(Eq("object"), Eq('?'), Eq(0), Eq(std::unordered_map<std::string, uint32_t>{})))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, addInitialAction(Eq("object"), Eq("action_1"), Eq(2), Eq(10), Eq(false)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, addInitialAction(Eq("object"), Eq("action_2"), Eq(0), Eq(0), Eq(true)))
      .Times(1);

  gdyFactory->loadObjects(objectsNode);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

void expectOpposingDefinitionNOP(ActionBehaviourType behaviourType, std::string sourceObjectName, std::string destinationObjectName, std::shared_ptr<MockObjectGenerator> mockObjectGeneratorPtr) {
  ActionBehaviourDefinition expectedNOPDefinition = GDYFactory::makeBehaviourDefinition(
      behaviourType == ActionBehaviourType::DESTINATION ? ActionBehaviourType::SOURCE : ActionBehaviourType::DESTINATION,
      behaviourType == ActionBehaviourType::DESTINATION ? sourceObjectName : destinationObjectName,
      behaviourType == ActionBehaviourType::SOURCE ? sourceObjectName : destinationObjectName,
      "action",
      "nop",
      {},
      {},
      {});

  auto objectName = behaviourType == ActionBehaviourType::SOURCE ? destinationObjectName : sourceObjectName;

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq(objectName), ActionBehaviourDefinitionEqMatcher(expectedNOPDefinition)))
      .Times(1);
}

void testBehaviourDefinition(std::string yamlString, ActionBehaviourDefinition expectedBehaviourDefinition, bool expectNOP) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  auto objectName = expectedBehaviourDefinition.behaviourType == ActionBehaviourType::SOURCE ? expectedBehaviourDefinition.sourceObjectName : expectedBehaviourDefinition.destinationObjectName;

  EXPECT_CALL(*mockObjectGeneratorPtr, defineActionBehaviour(Eq(objectName), ActionBehaviourDefinitionEqMatcher(expectedBehaviourDefinition)))
      .Times(1);

  if (expectNOP) {
    expectOpposingDefinitionNOP(expectedBehaviourDefinition.behaviourType, expectedBehaviourDefinition.sourceObjectName, expectedBehaviourDefinition.destinationObjectName, mockObjectGeneratorPtr);
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

TEST(GDYFactoryTest, loadAction_source_empty) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: _empty
          Commands: 
            - spawn: cars
        Dst:
          Object: _empty
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "_empty",
      "_empty",
      "action",
      "spawn",
      {{"0", _Y("cars")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_dest_empty) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: _empty
        Dst:
          Object: _empty
          Commands: 
            - spawn: cars
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "_empty",
      "_empty",
      "action",
      "spawn",
      {{"0", _Y("cars")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_destination_missing) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Src:
          Object: _empty
          Commands: 
            - spawn: cars
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::SOURCE,
      "_empty",
      "_empty",
      "action",
      "spawn",
      {{"0", _Y("cars")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

TEST(GDYFactoryTest, loadAction_src_missing) {
  auto yamlString = R"(
Actions:
  - Name: action
    Behaviours:
      - Dst:
          Object: _empty
          Commands: 
            - spawn: cars
)";

  ActionBehaviourDefinition expectedBehaviourDefinition = GDYFactory::makeBehaviourDefinition(
      ActionBehaviourType::DESTINATION,
      "_empty",
      "_empty",
      "action",
      "spawn",
      {{"0", _Y("cars")}},
      {},
      {});

  testBehaviourDefinition(yamlString, expectedBehaviourDefinition, true);
}

std::map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefs(std::vector<std::string> objectNames) {
  std::map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions;
  for (auto name : objectNames) {
    ObjectDefinition objectDefinition = {
        name};
    mockObjectDefinitions[name] = std::make_shared<ObjectDefinition>(objectDefinition);
  }

  return mockObjectDefinitions;
}

TEST(GDYFactoryTest, wallTest) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto grid = std::make_shared<Grid>();

  auto mockWall2Object = std::make_shared<MockObject>();
  auto mockWall16Object = std::make_shared<MockObject>();
  auto mockDefaultObject = std::make_shared<MockObject>();

  std::string wall2String = "Wall2";
  std::string wall16String = "Wall16";

  EXPECT_CALL(*mockWall2Object, getObjectName())
      .WillRepeatedly(ReturnRef(wall2String));

  EXPECT_CALL(*mockWall16Object, getObjectName())
      .WillRepeatedly(ReturnRef(wall16String));

  auto objectDefinitions = mockObjectDefs({wall2String, wall16String});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(ReturnRefOfCopy(objectDefinitions));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall2String));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .WillRepeatedly(ReturnRef(wall16String));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), Eq(1), Eq(grid)))
      .WillRepeatedly(Return(mockDefaultObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockDefaultObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall2String), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockWall2Object));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall16String), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockWall16Object));

  gdyFactory->initializeFromFile("tests/resources/walls.yaml");
  gdyFactory->getLevelGenerator(0)->reset(grid);

  ASSERT_EQ(grid->getWidth(), 17);
  ASSERT_EQ(grid->getHeight(), 17);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(GDYFactoryTest, zIndexTest) {
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));
  auto grid = std::make_shared<Grid>();

  auto mockWallObject = std::make_shared<MockObject>();
  auto mockFloorObject = std::make_shared<MockObject>();
  auto mockGhostObject = std::make_shared<MockObject>();

  auto mockDefaultObject = std::make_shared<MockObject>();

  std::string wall = "Wall2";
  std::string floor = "floor";
  std::string ghost = "ghost";

  EXPECT_CALL(*mockWallObject, getObjectName())
      .WillRepeatedly(ReturnRef(wall));

  EXPECT_CALL(*mockFloorObject, getObjectName())
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockGhostObject, getObjectName())
      .WillRepeatedly(ReturnRef(ghost));

  auto objectDefinitions = mockObjectDefs({wall, floor, ghost});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(ReturnRefOfCopy(objectDefinitions));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('*')))
      .WillRepeatedly(ReturnRef(wall));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('f')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('g')))
      .WillRepeatedly(ReturnRef(floor));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), Eq(1), Eq(grid)))
      .WillRepeatedly(Return(mockDefaultObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockDefaultObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wall), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(floor), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockFloorObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(ghost), Eq(0), Eq(grid)))
      .WillRepeatedly(Return(mockGhostObject));

  gdyFactory->initializeFromFile("tests/resources/ztest.yaml");
  gdyFactory->getLevelGenerator(0)->reset(grid);

  ASSERT_EQ(grid->getWidth(), 5);
  ASSERT_EQ(grid->getHeight(), 5);
}

MATCHER_P(ActionTriggerMatcherEq, expectedActionTriggerDefinitions, "") {
  if (expectedActionTriggerDefinitions.size() != arg.size()) {
    return false;
  }

  for (auto expectedActionTriggerDefinitionsPair : expectedActionTriggerDefinitions) {
    auto key = expectedActionTriggerDefinitionsPair.first;
    auto expectedActionTriggerDefinition = expectedActionTriggerDefinitionsPair.second;

    auto actualActionTriggerDefinitionIt = arg.find(key);
    if (actualActionTriggerDefinitionIt == arg.end()) {
      return false;
    }

    auto actualActionTriggerDefinition = actualActionTriggerDefinitionIt->second;

    if (expectedActionTriggerDefinition.sourceObjectNames != actualActionTriggerDefinition.sourceObjectNames) {
      return false;
    }

    if (expectedActionTriggerDefinition.destinationObjectNames != actualActionTriggerDefinition.destinationObjectNames) {
      return false;
    }

    if (expectedActionTriggerDefinition.triggerType != actualActionTriggerDefinition.triggerType) {
      return false;
    }

    if (expectedActionTriggerDefinition.range != actualActionTriggerDefinition.range) {
      return false;
    }
  }

  return true;
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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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

TEST(GDYFactoryTest, action_input_meta_data) {
  auto yamlString = R"(
Actions:
  - Name: move
    InputMapping:
      Inputs:
        1: 
          Description: Do Something
          OrientationVector: [1, 0]
          MetaData: 
            Image: 0
        2:
          VectorToDest: [0, -1]
          MetaData: 
            Image: 1
        4:
          OrientationVector: [0, 1]
          VectorToDest: [0, 1]
          MetaData: 
            Image: 2   
      Relative: true
)";

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"move", {{
                    {1, {{0, 0}, {1, 0}, "Do Something", {{"Image", 0}}}},
                    {2, {{0, -1}, {0, 0}, "", {{"Image", 1}}}},
                    {4, {{0, 1}, {0, 1}, "", {{"Image", 2}}}},
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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"spawn", {{}, false, false, true}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));
}

TEST(GDYFactoryTest, action_input_default_mapping) {
  auto yamlString = R"(
Actions:
  - Name: move
)";

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
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
}

TEST(GDYFactoryTest, action_range_trigger) {
  auto yamlString = R"(
Actions:
  - Name: action
    Probability: 0.4
    Trigger:
      Type: RANGE_BOX_BOUNDARY 
      Range: 3
    Behaviours:
      - Src:
          Object: sourceObject
        Dst:
          Object: destinationObject
          Commands:
            - decr: resources
      
)";

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

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

  // Internal should be true and there is no input mapping
  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"action", {{}, false, true}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));

  std::unordered_map<std::string, ActionTriggerDefinition> expectedTriggerDefinitions{
      {"action", {{"sourceObject"}, {"destinationObject"}, TriggerType::RANGE_BOX_BOUNDARY, 3}}};

  ASSERT_THAT(gdyFactory->getActionTriggerDefinitions(), ActionTriggerMatcherEq(expectedTriggerDefinitions));
}

TEST(GDYFactoryTest, action_range_default_trigger_type) {
  auto yamlString = R"(
Actions:
  - Name: action
    Probability: 0.7
    Trigger:
      Range: 3
    Behaviours:
      - Src:
          Object: sourceObject
        Dst:
          Object: destinationObject
          Commands:
            - decr: resources
      
)";

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

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

  // Internal should be true and there is no input mapping
  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"action", {{}, false, true}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));

  std::unordered_map<std::string, ActionTriggerDefinition> expectedTriggerDefinitions{
      {"action", {{"sourceObject"}, {"destinationObject"}, TriggerType::RANGE_BOX_AREA, 3}}};

  ASSERT_THAT(gdyFactory->getActionTriggerDefinitions(), ActionTriggerMatcherEq(expectedTriggerDefinitions));
}

TEST(GDYFactoryTest, action_no_triggers) {
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

  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockTerminationGeneratorPtr = std::make_shared<MockTerminationGenerator>();
  auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(mockObjectGeneratorPtr, mockTerminationGeneratorPtr, {}));

  auto actionsNode = loadFromStringAndGetNode(std::string(yamlString), "Actions");

  gdyFactory->loadActions(actionsNode);

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

  // Internal should be true and there is no input mapping
  std::unordered_map<std::string, ActionInputsDefinition> expectedInputMappings{
      {"action", {{
                      {1, {{-1, 0}, {-1, 0}, "Left"}},
                      {2, {{0, -1}, {0, -1}, "Up"}},
                      {3, {{1, 0}, {1, 0}, "Right"}},
                      {4, {{0, 1}, {0, 1}, "Down"}},
                  },
                  false,
                  false}}};

  ASSERT_THAT(gdyFactory->getActionInputsDefinitions(), InputMappingMatcherEq(expectedInputMappings));

  std::unordered_map<std::string, ActionTriggerDefinition> expectedTriggerDefinitions{};

  ASSERT_THAT(gdyFactory->getActionTriggerDefinitions(), ActionTriggerMatcherEq(expectedTriggerDefinitions));
}

}  // namespace griddly