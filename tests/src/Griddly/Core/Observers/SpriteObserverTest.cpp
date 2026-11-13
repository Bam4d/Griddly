#include <memory>

#include "Griddly/Core/Observers/SpriteObserver.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "ObserverMultiAgentTestData.hpp"
#include "ObserverRTSTestData.hpp"
#include "ObserverTestData.hpp"
#include "ObserverTestUtils.hpp"
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
                           std::vector<uint32_t> expectedObservationShape,
                           std::vector<uint32_t> expectedObservationStride,
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
    write_image(testName + ".png", &updateObservation, spriteObserver->getStrides()[2], spriteObserver->getShape()[1], spriteObserver->getShape()[2]);
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), &updateObservation));

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

TEST(SpriteObserverTest, render_player_observability_in_global_observer_highlight) {
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

  config.globalObserverAvatarMode = GlobalObserverAvatarMode::HIGHLIGHT_VISIBLE;

  config.resourceConfig = {"resources/games", "resources/images"};

  runSpriteObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/sprite/render_player_observability_in_global_observer_highlight.png");
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

  std::vector<uint32_t> expectedObservationShape = {3, 120, 120};
  std::vector<uint32_t> expectedObservationStride = {1, 4, 4 * 100};

  auto expectedImageData = loadExpectedImage("tests/resources/observer/sprite/defaultObserverConfig.png");

  // Reset and update 100 times to make sure reset is stable
  for (int x = 0; x < 100; x++) {
    spriteObserver->reset();

    auto& updateObservation = spriteObserver->update();

    ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
    ASSERT_EQ(spriteObserver->getStrides()[0], expectedObservationStride[0]);
    ASSERT_EQ(spriteObserver->getStrides()[1], expectedObservationStride[1]);

    ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(spriteObserver->getShape(), spriteObserver->getStrides(), &updateObservation));
  }

  size_t dataLength = 4 * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  testEnvironment.verifyAndClearExpectations();
}

TEST(SpriteObserverTest, strides_valid_before_first_update) {
  // Bindings read shape/strides around update(); they must already be valid straight after reset()
  SpriteObserverConfig observerConfig;
  observerConfig.tileSize = glm::ivec2(24, 24);
  observerConfig.resourceConfig.imagePath = "resources/images";
  observerConfig.spriteDefinitions = getMockSpriteDefinitions();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(Direction::NONE));
  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));
  spriteObserver->init({spriteObserver});

  spriteObserver->reset();
  ASSERT_EQ(spriteObserver->getShape(), (std::vector<uint32_t>{3, 120, 120}));
  ASSERT_EQ(spriteObserver->getStrides(), (std::vector<uint32_t>{1, 4, 4 * 120}));

  spriteObserver->update();
  ASSERT_EQ(spriteObserver->getStrides(), (std::vector<uint32_t>{1, 4, 4 * 120}));
}

// Renders the RTS test grid with tiny (1/10 tile) sprites and a highlight that covers exactly one
// tile, so the highlight around each object can be sampled; observer is player 1 (green), player 2 is red
std::shared_ptr<SpriteObserver> renderHighlightTest(HighlightStyle style, ObserverRTSTestData& testEnvironment, SpriteObserverConfig& observerConfig) {
  observerConfig.highlightStyle = style;
  auto spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(testEnvironment.mockGridPtr, observerConfig));
  spriteObserver->init({spriteObserver, spriteObserver, spriteObserver});
  spriteObserver->reset();
  spriteObserver->update();
  return spriteObserver;
}

SpriteObserverConfig highlightTestConfig() {
  SpriteObserverConfig observerConfig = {5, 5, 0, 0};
  observerConfig.playerId = 1;
  observerConfig.playerCount = 3;
  observerConfig.tileSize = glm::ivec2(50, 50);
  observerConfig.highlightPlayers = true;
  observerConfig.highlightScale = 10.0f;
  observerConfig.playerColors = {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
  observerConfig.resourceConfig.imagePath = "resources/images";
  auto spriteDefinitions = getMockRTSSpriteDefinitions();
  for (const auto& name : {"A0", "B0", "C0"}) {
    spriteDefinitions[name].scale = 0.1f;
  }
  observerConfig.spriteDefinitions = spriteDefinitions;
  return observerConfig;
}

TEST(SpriteObserverTest, multiPlayer_Glow_centred_on_object) {
  // GLOW highlights are a single player colour, fade out, and are centred on the object
  auto observerConfig = highlightTestConfig();
  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);
  auto spriteObserver = renderHighlightTest(HighlightStyle::GLOW, testEnvironment, observerConfig);

  const uint8_t* pixels = &spriteObserver->update();
  auto width = spriteObserver->getShape()[1];
  auto pixel = [&](uint32_t x, uint32_t y) { return pixels + (static_cast<size_t>(y) * width + x) * 4; };

  // A1 at (1,1) is owned by the observing player -> first colour (green)
  const uint32_t tile = 50;
  const uint32_t cx = 1 * tile + tile / 2, cy = 1 * tile + tile / 2;
  // just outside the 5px sprite: mostly green
  auto* core = pixel(cx + 6, cy);
  ASSERT_GT(core[1], 150);
  ASSERT_GT(core[1], core[0] + 100);
  ASSERT_GT(core[1], core[2] + 100);

  // fades towards the tile edge
  ASSERT_LT(pixel(1 * tile + 1, cy)[1] + 80, core[1]);

  // C2 at (3,2) belongs to player 2 -> second colour (red)
  auto* enemy = pixel(3 * tile + tile / 2 + 6, 2 * tile + tile / 2);
  ASSERT_GT(enemy[0], 150);
  ASSERT_GT(enemy[0], enemy[1] + 100);

  // symmetric about the object centre: mirrored samples match (sampled where the glow
  // dominates; at the faint outer edge the textured floor shows through)
  for (uint32_t d : {6u, 9u, 12u}) {
    auto* left = pixel(cx - d, cy);
    auto* right = pixel(cx + d - 1, cy);
    auto* up = pixel(cx, cy - d);
    auto* down = pixel(cx, cy + d - 1);
    ASSERT_NEAR(left[1], right[1], 8) << "horizontal asymmetry at d=" << d;
    ASSERT_NEAR(up[1], down[1], 8) << "vertical asymmetry at d=" << d;
  }
}

TEST(SpriteObserverTest, multiPlayer_Shadow_under_object) {
  // SHADOW highlights are the glow squashed into an ellipse centred on the bottom edge of the sprite
  auto observerConfig = highlightTestConfig();
  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);
  auto spriteObserver = renderHighlightTest(HighlightStyle::SHADOW, testEnvironment, observerConfig);

  const uint8_t* pixels = &spriteObserver->update();
  auto width = spriteObserver->getShape()[1];
  auto pixel = [&](uint32_t x, uint32_t y) { return pixels + (static_cast<size_t>(y) * width + x) * 4; };
  auto greenness = [&](uint32_t x, uint32_t y) { auto* p = pixel(x, y); return static_cast<int>(p[1]) - static_cast<int>(p[0]); };

  // A1 at (1,1): sprite is 5px tall, so its bottom edge is 2.5px below the tile centre
  const uint32_t tile = 50;
  const uint32_t cx = 1 * tile + tile / 2, cy = 1 * tile + tile / 2;
  const uint32_t shadowY = cy + 3;

  // strongly green on the shadow line beside the sprite, nothing above the sprite
  ASSERT_GT(greenness(cx + 6, shadowY), 100);
  ASSERT_LT(greenness(cx, cy - 8), 20);
  ASSERT_LT(greenness(cx + 6, cy - 8), 20);


  // wider than tall: still green 9px out sideways, but already gone 6px below the line
  ASSERT_GT(greenness(cx + 9, shadowY), 40);
  ASSERT_LT(greenness(cx, shadowY + 6), 20);

  // symmetric left / right (compared on green minus red, so the floor showing through mostly cancels)
  for (uint32_t d : {6u, 9u, 12u}) {
    ASSERT_NEAR(greenness(cx - d, shadowY), greenness(cx + d - 1, shadowY), 8) << "horizontal asymmetry at d=" << d;
  }

  // the enemy (C2 at (3,2), player 2) gets a red shadow
  auto* enemy = pixel(3 * tile + tile / 2 + 6, 2 * tile + tile / 2 + 3);
  ASSERT_GT(enemy[0], enemy[1] + 100);
}

}  // namespace griddly
