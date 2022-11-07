#include <memory>

#include "Griddly/Core/Observers/IsometricSpriteObserver.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "ObserverRTSTestData.hpp"
#include "ObserverTestData.hpp"
#include "VulkanObserverTest.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

std::map<std::string, SpriteDefinition> getMockRTSIsometricSpriteDefinitions() {
  // mock wall object
  SpriteDefinition mockObject1SpriteDefinition;
  mockObject1SpriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
  mockObject1SpriteDefinition.offset = glm::ivec2(0, 3);
  mockObject1SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/lava-1.png",
  };

  // mock object 2
  SpriteDefinition mockObject2SpriteDefinition;
  mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject2SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/crate-1.png",
  };

  // mock object 3
  SpriteDefinition mockObject3SpriteDefinition;
  mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject3SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/bush-1.png",
  };

  // mock avatar 3
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/avatars/spider-1.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/grass-1.png",
  };

  return {
      {"_iso_background_", backgroundSpriteDefinition},
      {"W0", mockObject1SpriteDefinition},
      {"A0", mockAvatarSpriteDefinition},
      {"B0", mockObject2SpriteDefinition},
      {"C0", mockObject3SpriteDefinition},
  };
}

void runIsometricSpriteObserverRTSTest(IsometricSpriteObserverConfig observerConfig,
                                       std::vector<int64_t> expectedObservationShape,
                                       std::vector<int64_t> expectedObservationStride,
                                       std::string expectedOutputFilename,
                                       bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;
  observerConfig.highlightPlayers = true;

  observerConfig.resourceConfig = {"resources/games", "resources/images", "resources/shaders"};
  observerConfig.shaderVariableConfig = ShaderVariableConfig();

  observerConfig.spriteDefinitions = getMockRTSIsometricSpriteDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(testEnvironment.mockGridPtr));

  isometricObserver->init(observerConfig);
  isometricObserver->reset();

  auto& updateObservation = isometricObserver->update();

  ASSERT_EQ(isometricObserver->getTileSize(), glm::ivec2(32, 48));
  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data, isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data));

  testEnvironment.verifyAndClearExpectations();
}

std::map<std::string, SpriteDefinition> getMockIsometricSpriteDefinitions() {
  // mock object 1
  SpriteDefinition mockObject1SpriteDefinition;
  mockObject1SpriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
  mockObject1SpriteDefinition.offset = glm::ivec2(0, 3);
  mockObject1SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/lava-1.png",
  };

  // mock object 2
  SpriteDefinition mockObject2SpriteDefinition;
  mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject2SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/crate-1.png",
  };

  // mock object 3
  SpriteDefinition mockObject3SpriteDefinition;
  mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject3SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/bush-1.png",
  };

  // mock avatar 3
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/avatars/spider-1.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/grass-1.png",
  };

  return {
      {"_iso_background_", backgroundSpriteDefinition},
      {"avatar0", mockAvatarSpriteDefinition},
      {"mo10", mockObject1SpriteDefinition},
      {"mo20", mockObject2SpriteDefinition},
      {"mo30", mockObject3SpriteDefinition},
  };
}

void runIsometricSpriteObserverTest(IsometricSpriteObserverConfig observerConfig,
                                    Direction avatarDirection,
                                    std::vector<int64_t> expectedObservationShape,
                                    std::vector<int64_t> expectedObservationStride,
                                    std::string expectedOutputFilename,
                                    bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;

  observerConfig.spriteDefinitions = getMockIsometricSpriteDefinitions();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(testEnvironment.mockGridPtr));

  isometricObserver->init(observerConfig);
  isometricObserver->reset();

  if (observerConfig.trackAvatar) {
    isometricObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }

  auto& updateObservation = isometricObserver->update();

  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data, isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data));

  testEnvironment.verifyAndClearExpectations();
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      -1,
      false, false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_NONE) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_NONE.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_UP) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_UP.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_RIGHT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_DOWN.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_LEFT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_NONE.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_UP.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_RIGHT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_DOWN.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_LEFT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  IsometricSpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(IsometricSpriteObserverTest, object_variable_fragment_lighting) {
  IsometricSpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  config.shaderVariableConfig = {
      {"_steps"},
      {"light"},
  };

  config.resourceConfig = {"resources/games", "resources/images", "tests/resources/observer/isometric/shaders/lighting"};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/object_variable_fragment_lighting.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player1) {
  IsometricSpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player1.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player2) {
  IsometricSpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player2.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player3) {
  IsometricSpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player3.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Global) {
  IsometricSpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Global.png");
}

TEST(IsometricSpriteObserverTest, reset) {
  IsometricSpriteObserverConfig observerConfig;
  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;

  observerConfig.resourceConfig = {"resources/games", "resources/images", "resources/shaders"};
  observerConfig.shaderVariableConfig = ShaderVariableConfig();

  observerConfig.spriteDefinitions = getMockIsometricSpriteDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(Direction::NONE));

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(testEnvironment.mockGridPtr));

  isometricObserver->init(observerConfig);

  std::vector<int64_t> expectedObservationShape = {3, 160, 128};
  std::vector<int64_t> expectedObservationStride = {1, 4, 4 * 160};

  auto expectedImageData = loadExpectedImage("tests/resources/observer/isometric/defaultObserverConfig.png");

  // Reset and update 100 times to make sure reset is stable
  for (int x = 0; x < 100; x++) {
    isometricObserver->reset();

    auto& updateObservation = isometricObserver->update();

    ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
    ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
    ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

    ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data));
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  testEnvironment.verifyAndClearExpectations();
}

}  // namespace griddly
