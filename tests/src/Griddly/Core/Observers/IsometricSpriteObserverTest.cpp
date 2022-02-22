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

std::unordered_map<std::string, SpriteDefinition> getMockRTSIsometricSpriteDefinitions() {
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

void runIsometricSpriteObserverRTSTest(ObserverConfig observerConfig,
                                       std::vector<uint32_t> expectedObservationShape,
                                       std::vector<uint32_t> expectedObservationStride,
                                       std::string expectedOutputFilename,
                                       bool writeOutputFile = false) {
  ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};

  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;
  observerConfig.highlightPlayers = true;

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::make_shared<IsometricSpriteObserver>(testEnvironment.mockGridPtr, resourceConfig, getMockRTSIsometricSpriteDefinitions(), ShaderVariableConfig());

  isometricObserver->init(observerConfig);
  isometricObserver->reset();

  auto updateObservation = isometricObserver->update();

  ASSERT_EQ(isometricObserver->getTileSize(), glm::ivec2(32, 48));
  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", updateObservation, isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

std::unordered_map<std::string, SpriteDefinition> getMockIsometricSpriteDefinitions() {
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

void runIsometricSpriteObserverTest(ObserverConfig observerConfig,
                                    Direction avatarDirection,
                                    std::vector<uint32_t> expectedObservationShape,
                                    std::vector<uint32_t> expectedObservationStride,
                                    std::string expectedOutputFilename,
                                    bool trackAvatar,
                                    ShaderVariableConfig shaderVariableConfig = ShaderVariableConfig(),
                                    ResourceConfig resourceConfig = {"resources/images", "resources/shaders"},
                                    bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection), trackAvatar);

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::make_shared<IsometricSpriteObserver>(testEnvironment.mockGridPtr, resourceConfig, getMockIsometricSpriteDefinitions(), shaderVariableConfig);

  isometricObserver->init(observerConfig);
  isometricObserver->reset();

  if (trackAvatar) {
    isometricObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }

  auto updateObservation = isometricObserver->update();

  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", updateObservation, isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", false);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      5,
      3,
      0,
      -1,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 112}, {1, 4, 4 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, object_variable_fragment_lighting) {
  ShaderVariableConfig shaderVariableConfig = {
      {"_steps"},
      {"light"},
  };

  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  ResourceConfig resourceConfig = {"resources/images", "tests/resources/observer/isometric/shaders/lighting"};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/object_variable_fragment_lighting.png", true, shaderVariableConfig, resourceConfig);
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player1) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player1.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player2) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player2.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player3) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player3.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Global) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  runIsometricSpriteObserverRTSTest(config, {3, 160, 128}, {1, 4, 4 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Global.png");
}

TEST(IsometricSpriteObserverTest, reset) {
  ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};
  ObserverConfig observerConfig;

  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(Direction::NONE), false);

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::make_shared<IsometricSpriteObserver>(testEnvironment.mockGridPtr, resourceConfig, getMockIsometricSpriteDefinitions(), ShaderVariableConfig());

  isometricObserver->init(observerConfig);

  std::vector<uint32_t> expectedObservationShape = {3, 160, 128};
  std::vector<uint32_t> expectedObservationStride = {1, 4, 4 * 160};

  auto expectedImageData = loadExpectedImage("tests/resources/observer/isometric/defaultObserverConfig.png");

  // Reset and update 100 times to make sure reset is stable
  for (int x = 0; x < 100; x++) {
    isometricObserver->reset();

    auto updateObservation = isometricObserver->update();

    ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
    ASSERT_EQ(isometricObserver->getStrides()[0], expectedObservationStride[0]);
    ASSERT_EQ(isometricObserver->getStrides()[1], expectedObservationStride[1]);

    ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(isometricObserver->getShape(), isometricObserver->getStrides(), updateObservation));
  }

  size_t dataLength = 4 * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  testEnvironment.verifyAndClearExpectations();
}

}  // namespace griddly