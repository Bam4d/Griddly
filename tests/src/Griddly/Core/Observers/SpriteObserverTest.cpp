#include <memory>

#include "Griddly/Core/Observers/SpriteObserver.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "ObserverMultiAgentTestData.hpp"
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

std::map<std::string, SpriteDefinition> getMockSpriteDefinitions() {
  // mock object 1
  SpriteDefinition mockObject1SpriteDefinition;
  mockObject1SpriteDefinition.tilingMode = TilingMode::WALL_16;
  mockObject1SpriteDefinition.images = {
      "gvgai/oryx/dirtWall_0.png",
      "gvgai/oryx/dirtWall_1.png",
      "gvgai/oryx/dirtWall_2.png",
      "gvgai/oryx/dirtWall_3.png",
      "gvgai/oryx/dirtWall_4.png",
      "gvgai/oryx/dirtWall_5.png",
      "gvgai/oryx/dirtWall_6.png",
      "gvgai/oryx/dirtWall_7.png",
      "gvgai/oryx/dirtWall_8.png",
      "gvgai/oryx/dirtWall_9.png",
      "gvgai/oryx/dirtWall_10.png",
      "gvgai/oryx/dirtWall_11.png",
      "gvgai/oryx/dirtWall_12.png",
      "gvgai/oryx/dirtWall_13.png",
      "gvgai/oryx/dirtWall_14.png",
      "gvgai/oryx/dirtWall_15.png",
  };

  // mock object 2
  SpriteDefinition mockObject2SpriteDefinition;
  mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject2SpriteDefinition.images = {
      "gvgai/oryx/bush2.png",
  };

  // mock object 3
  SpriteDefinition mockObject3SpriteDefinition;
  mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject3SpriteDefinition.images = {
      "gvgai/oryx/bear3.png",
  };

  // mock avatar 3
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "gvgai/oryx/spelunky_0.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "gvgai/oryx/floor2.png",
  };

  return {
      {"_background_", backgroundSpriteDefinition},
      {"avatar0", mockAvatarSpriteDefinition},
      {"mo10", mockObject1SpriteDefinition},
      {"mo20", mockObject2SpriteDefinition},
      {"mo30", mockObject3SpriteDefinition},
  };
}

std::map<std::string, SpriteDefinition> getMockRTSSpriteDefinitions() {
  // mock wall object
  SpriteDefinition mockObjectWallSpriteDefinition;
  mockObjectWallSpriteDefinition.tilingMode = TilingMode::WALL_16;
  mockObjectWallSpriteDefinition.images = {
      "oryx/oryx_fantasy/wall4-0.png",
      "oryx/oryx_fantasy/wall4-1.png",
      "oryx/oryx_fantasy/wall4-2.png",
      "oryx/oryx_fantasy/wall4-3.png",
      "oryx/oryx_fantasy/wall4-4.png",
      "oryx/oryx_fantasy/wall4-5.png",
      "oryx/oryx_fantasy/wall4-6.png",
      "oryx/oryx_fantasy/wall4-7.png",
      "oryx/oryx_fantasy/wall4-8.png",
      "oryx/oryx_fantasy/wall4-9.png",
      "oryx/oryx_fantasy/wall4-10.png",
      "oryx/oryx_fantasy/wall4-11.png",
      "oryx/oryx_fantasy/wall4-12.png",
      "oryx/oryx_fantasy/wall4-13.png",
      "oryx/oryx_fantasy/wall4-14.png",
      "oryx/oryx_fantasy/wall4-15.png",
  };

  // mock object A
  SpriteDefinition mockObjectASpriteDefinition;
  mockObjectASpriteDefinition.tilingMode = TilingMode::NONE;
  mockObjectASpriteDefinition.images = {
      "oryx/oryx_fantasy/avatars/gnome1.png",
  };

  // mock object B
  SpriteDefinition mockObjectBSpriteDefinition;
  mockObjectBSpriteDefinition.tilingMode = TilingMode::NONE;
  mockObjectBSpriteDefinition.images = {
      "oryx/oryx_fantasy/avatars/spider1.png",
  };

  // mock object C
  SpriteDefinition mockObjectCSpriteDefinition;
  mockObjectCSpriteDefinition.tilingMode = TilingMode::NONE;
  mockObjectCSpriteDefinition.images = {
      "oryx/oryx_fantasy/avatars/priest1.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "oryx/oryx_fantasy/floor4-2.png",
  };

  return {
      {"_background_", backgroundSpriteDefinition},
      {"W0", mockObjectWallSpriteDefinition},
      {"A0", mockObjectASpriteDefinition},
      {"B0", mockObjectBSpriteDefinition},
      {"C0", mockObjectCSpriteDefinition},
  };
}

std::map<std::string, SpriteDefinition> getMockSpriteMultiAgentDefinitions() {
  // mock object 1
  SpriteDefinition mockWallSpriteDefinition;
  mockWallSpriteDefinition.tilingMode = TilingMode::WALL_16;
  mockWallSpriteDefinition.images = {
      "gvgai/oryx/dirtWall_0.png",
      "gvgai/oryx/dirtWall_1.png",
      "gvgai/oryx/dirtWall_2.png",
      "gvgai/oryx/dirtWall_3.png",
      "gvgai/oryx/dirtWall_4.png",
      "gvgai/oryx/dirtWall_5.png",
      "gvgai/oryx/dirtWall_6.png",
      "gvgai/oryx/dirtWall_7.png",
      "gvgai/oryx/dirtWall_8.png",
      "gvgai/oryx/dirtWall_9.png",
      "gvgai/oryx/dirtWall_10.png",
      "gvgai/oryx/dirtWall_11.png",
      "gvgai/oryx/dirtWall_12.png",
      "gvgai/oryx/dirtWall_13.png",
      "gvgai/oryx/dirtWall_14.png",
      "gvgai/oryx/dirtWall_15.png",
  };

  // mock avatar
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "gvgai/oryx/spelunky_0.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "gvgai/oryx/floor2.png",
  };

  return {
      {"_background_", backgroundSpriteDefinition},
      {"avatar0", mockAvatarSpriteDefinition},
      {"wall0", mockWallSpriteDefinition},
  };
}

void runSpriteObserverTest(SpriteObserverConfig observerConfig,
                           Direction avatarDirection,
                           std::vector<int64_t> expectedObservationShape,
                           std::vector<int64_t> expectedObservationStride,
                           std::string expectedOutputFilename,
                           bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(24, 24);

  observerConfig.playerCount = 1;

  observerConfig.resourceConfig.imagePath = "resources/images";
  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/sprite";
  }

  observerConfig.spriteDefinitions = getMockSpriteDefinitions();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));

  spriteObserver->init({spriteObserver});
  if (observerConfig.trackAvatar) {
    spriteObserver->reset(testEnvironment.mockAvatarObjectPtr);
  } else {
    spriteObserver->reset();
  }

  auto& updateObservation = spriteObserver->update();

  ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(spriteObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(spriteObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", &updateObservation, spriteObserver->getStrides()[2], spriteObserver->getShape()[1], spriteObserver->getShape()[2]);
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), &updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

void runSpriteObserverRTSTest(SpriteObserverConfig observerConfig,
                              std::vector<uint32_t> expectedObservationShape,
                              std::vector<uint32_t> expectedObservationStride,
                              std::string expectedOutputFilename,
                              bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(50, 50);
  observerConfig.highlightPlayers = true;
  observerConfig.playerCount = 3;

  observerConfig.resourceConfig.imagePath = "resources/images";
  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/sprite";
  }

  observerConfig.shaderVariableConfig = ShaderVariableConfig();

  observerConfig.spriteDefinitions = getMockRTSSpriteDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));

  spriteObserver->init({spriteObserver, spriteObserver, spriteObserver});
  spriteObserver->reset();

  auto& updateObservation = spriteObserver->update();

  ASSERT_EQ(spriteObserver->getTileSize(), glm::ivec2(50, 50));
  ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(spriteObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(spriteObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", &updateObservation, spriteObserver->getStrides()[2], spriteObserver->getShape()[1], spriteObserver->getShape()[2]);
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), &updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

void runSpriteObserverMultiAgentTest(SpriteObserverConfig observerConfig,
                                     std::vector<uint32_t> expectedObservationShape,
                                     std::vector<uint32_t> expectedObservationStride,
                                     std::string expectedOutputFilename,
                                     bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(20, 20);
  observerConfig.highlightPlayers = true;
  observerConfig.playerCount = 4;

  observerConfig.resourceConfig.imagePath = "resources/images";
  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/sprite";
  }

  observerConfig.spriteDefinitions = getMockSpriteMultiAgentDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverMultiAgentTestData testEnvironment = ObserverMultiAgentTestData(observerConfig);

  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));

  // We have 4 players so we should put 3 observers here
  spriteObserver->init({testEnvironment.mockAgent1ObserverPtr, testEnvironment.mockAgent2ObserverPtr, testEnvironment.mockAgent3ObserverPtr, testEnvironment.mockAgent4ObserverPtr});
  spriteObserver->reset();

  auto& updateObservation = spriteObserver->update();

  ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(spriteObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(spriteObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data, spriteObserver->getStrides()[2], spriteObserver->getShape()[1], spriteObserver->getShape()[2]);
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data));

  testEnvironment.verifyAndClearExpectations();
}

TEST(SpriteObserverTest, defaultObserverConfig) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};
  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      true, true};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 120}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(SpriteObserverTest, partialObserver) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      -1,
      false, false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset.png");
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_NONE) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_NONE.png");
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_UP) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_UP.png");
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_RIGHT.png");
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_DOWN.png");
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_LEFT.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_NONE.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_UP.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_RIGHT.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_DOWN.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      false, true};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_LEFT.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  SpriteObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 4, 4 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(SpriteObserverTest, object_variable_health_bars) {
  SpriteObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  config.shaderVariableConfig = {
      {"_steps"},
      {"health", "max_health"},
  };

  config.resourceConfig = {"resources/games", "resources/images", "tests/resources/observer/sprite/shaders/health_bars"};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 120}, {1, 4, 4 * 100}, "tests/resources/observer/sprite/object_variable_health_bars.png");
}

TEST(SpriteObserverTest, render_player_observability_in_global_observer_greyscale) {
  SpriteObserverConfig config = {
      0,
      0,
      0,
      0,
      false,
      false};

  config.shaderVariableConfig = {
      {"_steps"},
      {},
  };

  config.globalObserverAvatarMode = GlobalObserverAvatarMode::GRAYSCALE_INVISIBLE;

  config.resourceConfig = {"resources/games", "resources/images"};

  runSpriteObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/sprite/render_player_observability_in_global_observer_greyscale.png");
}

TEST(SpriteObserverTest, render_player_observability_in_global_observer_darken) {
  SpriteObserverConfig config = {
      0,
      0,
      0,
      0,
      false,
      false};

  config.shaderVariableConfig = {
      {"_steps"},
      {},
  };

  config.globalObserverAvatarMode = GlobalObserverAvatarMode::DARKEN_INVISIBLE;

  config.resourceConfig = {"resources/games", "resources/images"};

  runSpriteObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/sprite/render_player_observability_in_global_observer_darken.png");
}

TEST(SpriteObserverTest, render_player_observability_in_global_observer_remove) {
  SpriteObserverConfig config = {
      0,
      0,
      0,
      0,
      false,
      false};

  config.shaderVariableConfig = {
      {"_steps"},
      {},
  };

  config.globalObserverAvatarMode = GlobalObserverAvatarMode::REMOVE_INVISIBLE;

  config.resourceConfig = {"resources/games", "resources/images"};

  runSpriteObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/sprite/render_player_observability_in_global_observer_remove.png");
}

TEST(SpriteObserverTest, multiPlayer_Outline_Player1) {
  SpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  runSpriteObserverRTSTest(config, {3, 250, 250}, {1, 4, 4 * 250}, "tests/resources/observer/sprite/multiPlayer_Outline_Player1.png");
}

TEST(SpriteObserverTest, multiPlayer_Outline_Player2) {
  SpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  runSpriteObserverRTSTest(config, {3, 250, 250}, {1, 4, 4 * 250}, "tests/resources/observer/sprite/multiPlayer_Outline_Player2.png");
}

TEST(SpriteObserverTest, multiPlayer_Outline_Player3) {
  SpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  runSpriteObserverRTSTest(config, {3, 250, 250}, {1, 4, 4 * 250}, "tests/resources/observer/sprite/multiPlayer_Outline_Player3.png");
}

TEST(SpriteObserverTest, multiPlayer_Outline_Global) {
  SpriteObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  runSpriteObserverRTSTest(config, {3, 250, 250}, {1, 4, 4 * 250}, "tests/resources/observer/sprite/multiPlayer_Outline_Global.png");
}

TEST(SpriteObserverTest, reset) {
  SpriteObserverConfig observerConfig;
  observerConfig.tileSize = glm::ivec2(24, 24);

  observerConfig.trackAvatar = false;
  observerConfig.resourceConfig.imagePath = "resources/images";
  observerConfig.resourceConfig.shaderPath = "resources/shaders/default/sprite";
  observerConfig.spriteDefinitions = getMockSpriteDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(Direction::NONE));

  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));

  spriteObserver->init({spriteObserver});

  std::vector<int64_t> expectedObservationShape = {3, 120, 120};
  std::vector<int64_t> expectedObservationStride = {1, 4, 4 * 100};

  auto expectedImageData = loadExpectedImage("tests/resources/observer/sprite/defaultObserverConfig.png");

  // Reset and update 100 times to make sure reset is stable
  for (int x = 0; x < 100; x++) {
    spriteObserver->reset();

    auto& updateObservation = spriteObserver->update();

    ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
    ASSERT_EQ(spriteObserver->getStrides()[0], expectedObservationStride[0]);
    ASSERT_EQ(spriteObserver->getStrides()[1], expectedObservationStride[1]);

    ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), (uint8_t *)updateObservation->getDLTensor()->dl_tensor.data));
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  testEnvironment.verifyAndClearExpectations();
}

}  // namespace griddly
