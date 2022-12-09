
#include <memory>

#include "Griddly/Core/Observers/BlockObserver.hpp"
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

std::map<std::string, BlockDefinition> getMockRTSBlockDefinitions() {
  float red[]{0.5, 0.2, 0.2};
  float green[]{0.2, 0.5, 0.2};
  float blue[]{0.2, 0.2, 0.5};
  float grey[]{0.3, 0.3, 0.3};

  // mock object 1
  BlockDefinition mockObjectABlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectABlockDefinition.color[c] = red[c];
  }
  mockObjectABlockDefinition.shape = "square";
  mockObjectABlockDefinition.scale = 0.8f;
  mockObjectABlockDefinition.outlineScale = 4.0f;

  // mock object 2
  BlockDefinition mockObjectBBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectBBlockDefinition.color[c] = green[c];
  }
  mockObjectBBlockDefinition.shape = "triangle";
  mockObjectBBlockDefinition.scale = 0.4f;
  mockObjectBBlockDefinition.outlineScale = 4.0f;

  // mock object 3
  BlockDefinition mockObjectCBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectCBlockDefinition.color[c] = blue[c];
  }
  mockObjectCBlockDefinition.shape = "circle";
  mockObjectCBlockDefinition.scale = 0.5f;
  mockObjectCBlockDefinition.outlineScale = 4.0f;

  // mock avatar 3
  BlockDefinition mockObjectWallBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectWallBlockDefinition.color[c] = grey[c];
  }
  mockObjectWallBlockDefinition.shape = "pentagon";
  mockObjectWallBlockDefinition.scale = 1.0f;

  return {
      {"W0", mockObjectWallBlockDefinition},
      {"A0", mockObjectABlockDefinition},
      {"B0", mockObjectBBlockDefinition},
      {"C0", mockObjectCBlockDefinition},
  };
}

std::map<std::string, BlockDefinition> getMockBlockDefinitions() {
  float red[]{1.0, 0.0, 0.0};
  float green[]{0.0, 1.0, 0.0};
  float blue[]{0.0, 0.0, 1.0};

  float white[]{1.0, 1.0, 1.0};

  std::string square = "square";
  std::string triangle = "triangle";

  // mock object 1
  BlockDefinition mockObject1BlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObject1BlockDefinition.color[c] = red[c];
  }
  mockObject1BlockDefinition.shape = square;
  mockObject1BlockDefinition.scale = 1.0f;

  // mock object 2
  BlockDefinition mockObject2BlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObject2BlockDefinition.color[c] = green[c];
  }
  mockObject2BlockDefinition.shape = triangle;
  mockObject2BlockDefinition.scale = 0.3f;

  // mock object 3
  BlockDefinition mockObject3BlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObject3BlockDefinition.color[c] = green[c];
  }
  mockObject3BlockDefinition.shape = square;
  mockObject3BlockDefinition.scale = 0.7f;

  // mock avatar
  BlockDefinition mockAvatarBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockAvatarBlockDefinition.color[c] = white[c];
  }
  mockAvatarBlockDefinition.shape = triangle;
  mockAvatarBlockDefinition.scale = 1.0f;

  return {
      {"avatar0", mockAvatarBlockDefinition},
      {"mo10", mockObject1BlockDefinition},
      {"mo20", mockObject2BlockDefinition},
      {"mo30", mockObject3BlockDefinition},
  };
}

std::map<std::string, BlockDefinition> getMockBlockMultiAgentDefinitions() {
  float red[]{1.0, 0.0, 0.0};
  float green[]{0.0, 1.0, 0.0};
  float blue[]{0.0, 0.0, 1.0};

  float white[]{1.0, 1.0, 1.0};

  std::string square = "square";
  std::string triangle = "triangle";

  // wall
  BlockDefinition mockWallBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockWallBlockDefinition.color[c] = green[c];
  }
  mockWallBlockDefinition.shape = square;
  mockWallBlockDefinition.scale = 0.7f;

  // mock avatar
  BlockDefinition mockAvatarBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockAvatarBlockDefinition.color[c] = white[c];
  }
  mockAvatarBlockDefinition.shape = triangle;
  mockAvatarBlockDefinition.scale = 1.0f;

  return {
      {"avatar0", mockAvatarBlockDefinition},
      {"wall0", mockWallBlockDefinition},
  };
}

void runBlockObserverTest(BlockObserverConfig observerConfig,
                          Direction avatarDirection,
                          std::vector<uint32_t> expectedObservationShape,
                          std::vector<uint32_t> expectedObservationStride,
                          std::string expectedOutputFilename,
                          bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(20, 20);

  observerConfig.playerCount = 1;

  observerConfig.resourceConfig.imagePath = "resources/images";

  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/block";
  }

  observerConfig.blockDefinitions = getMockBlockDefinitions();
  observerConfig.spriteDefinitions = BlockObserver::blockSpriteDefinitions_;

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(testEnvironment.mockGridPtr, observerConfig));

  blockObserver->init({blockObserver});
  if (observerConfig.trackAvatar) {
    blockObserver->reset(testEnvironment.mockAvatarObjectPtr);
  } else {
    blockObserver->reset();
  }

  auto& updateObservation = blockObserver->update();

  ASSERT_EQ(blockObserver->getTileSize(), glm::ivec2(20, 20));
  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(blockObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", &updateObservation, blockObserver->getStrides()[2], blockObserver->getShape()[1], blockObserver->getShape()[2]);
  }

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(blockObserver->getShape(), blockObserver->getStrides(), &updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

void runBlockObserverRTSTest(BlockObserverConfig observerConfig,
                             std::vector<uint32_t> expectedObservationShape,
                             std::vector<uint32_t> expectedObservationStride,
                             std::string expectedOutputFilename,
                             bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(20, 20);
  observerConfig.highlightPlayers = true;
  observerConfig.playerCount = 3;

  observerConfig.resourceConfig.imagePath = "resources/images";
  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/block";
  }

  observerConfig.blockDefinitions = getMockRTSBlockDefinitions();
  observerConfig.spriteDefinitions = BlockObserver::blockSpriteDefinitions_;

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(testEnvironment.mockGridPtr, observerConfig));

  // We have 3 players so we should put 3 observers here
  blockObserver->init({blockObserver, blockObserver, blockObserver});
  blockObserver->reset();

  auto& updateObservation = blockObserver->update();

  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(blockObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", &updateObservation, blockObserver->getStrides()[2], blockObserver->getShape()[1], blockObserver->getShape()[2]);
  }

  size_t dataLength = 4 * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(blockObserver->getShape(), blockObserver->getStrides(), &updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

void runBlockObserverMultiAgentTest(BlockObserverConfig observerConfig,
                                    std::vector<uint32_t> expectedObservationShape,
                                    std::vector<uint32_t> expectedObservationStride,
                                    std::string expectedOutputFilename,
                                    bool writeOutputFile = false) {
  observerConfig.tileSize = glm::ivec2(20, 20);
  observerConfig.highlightPlayers = true;
  observerConfig.playerCount = 4;

  observerConfig.resourceConfig.imagePath = "resources/images";
  if (observerConfig.resourceConfig.shaderPath.length() == 0) {
    observerConfig.resourceConfig.shaderPath = "resources/shaders/default/block";
  }

  observerConfig.blockDefinitions = getMockBlockMultiAgentDefinitions();
  observerConfig.spriteDefinitions = BlockObserver::blockSpriteDefinitions_;

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverMultiAgentTestData testEnvironment = ObserverMultiAgentTestData(observerConfig);

  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(testEnvironment.mockGridPtr, observerConfig));

  // We have 4 players so we should put 3 observers here
  blockObserver->init({testEnvironment.mockAgent1ObserverPtr, testEnvironment.mockAgent2ObserverPtr, testEnvironment.mockAgent3ObserverPtr, testEnvironment.mockAgent4ObserverPtr});
  blockObserver->reset();

  auto& updateObservation = blockObserver->update();

  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(blockObserver->getStrides()[1], expectedObservationStride[1]);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", &updateObservation, blockObserver->getStrides()[2], blockObserver->getShape()[1], blockObserver->getShape()[2]);
  }

  size_t dataLength = 4 * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(blockObserver->getShape(), blockObserver->getStrides(), &updateObservation));

  testEnvironment.verifyAndClearExpectations();
}

TEST(BlockObserverTest, defaultObserverConfig) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(BlockObserverTest, partialObserver) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver.png");
}

TEST(BlockObserverTest, partialObserver_withOffset) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      -1,
      false,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset.png");
}

TEST(BlockObserverTest, partialObserver_trackAvatar_NONE) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_NONE.png");
}

TEST(BlockObserverTest, partialObserver_trackAvatar_UP) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_UP.png");
}

TEST(BlockObserverTest, partialObserver_trackAvatar_RIGHT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_RIGHT.png");
}

TEST(BlockObserverTest, partialObserver_trackAvatar_DOWN) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_DOWN.png");
}

TEST(BlockObserverTest, partialObserver_trackAvatar_LEFT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_LEFT.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_NONE.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_UP.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_RIGHT.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_DOWN.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_LEFT.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png");
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  BlockObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 4, 4 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png");
}

TEST(BlockObserverTest, global_variable_lighting) {
  BlockObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  config.shaderVariableConfig = {
      {"_steps", "lightingR", "lightingG", "lightingB"},
      {},
  };

  config.resourceConfig = {"resources/games", "resources/images", "tests/resources/observer/block/shaders/global_lighting"};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/global_variable_lighting.png");
}

TEST(BlockObserverTest, render_player_observability_in_global_observer_greyscale) {
  BlockObserverConfig config = {
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

  runBlockObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/block/render_player_observability_in_global_observer_greyscale.png");
}

TEST(BlockObserverTest, render_player_observability_in_global_observer_darken) {
  BlockObserverConfig config = {
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

  runBlockObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/block/render_player_observability_in_global_observer_darken.png");
}

TEST(BlockObserverTest, render_player_observability_in_global_observer_remove) {
  BlockObserverConfig config = {
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

  runBlockObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/block/render_player_observability_in_global_observer_remove.png");
}

TEST(BlockObserverTest, render_player_observability_in_global_observer_highlight) {
  BlockObserverConfig config = {
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

  runBlockObserverMultiAgentTest(config, {3, 200, 200}, {1, 4, 4 * 200}, "tests/resources/observer/block/render_player_observability_in_global_observer_highlight.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Player1) {
  BlockObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/multiPlayer_Outline_Player1.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Player2) {
  BlockObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/multiPlayer_Outline_Player2.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Player3) {
  BlockObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/multiPlayer_Outline_Player3.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Global) {
  BlockObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 100, 100}, {1, 4, 4 * 100}, "tests/resources/observer/block/multiPlayer_Outline_Global.png");
}

TEST(BlockObserverTest, reset) {
  BlockObserverConfig observerConfig;
  observerConfig.tileSize = glm::ivec2(20, 20);

  observerConfig.resourceConfig.imagePath = "resources/images";
  observerConfig.resourceConfig.shaderPath = "resources/shaders/default/block";
  observerConfig.shaderVariableConfig = ShaderVariableConfig();
  observerConfig.spriteDefinitions = BlockObserver::blockSpriteDefinitions_;
  observerConfig.blockDefinitions = getMockBlockDefinitions();

  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(Direction::NONE));

  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(testEnvironment.mockGridPtr, observerConfig));

  blockObserver->init({blockObserver, blockObserver, blockObserver});

  std::vector<uint32_t> expectedObservationShape = {3, 100, 100};
  std::vector<uint32_t> expectedObservationStride = {1, 4, 4 * 100};

  auto expectedImageData = loadExpectedImage("tests/resources/observer/block/defaultObserverConfig.png");

  // Reset and update 100 times to make sure reset is stable
  for (int x = 0; x < 100; x++) {
    blockObserver->reset();

    auto& updateObservation = blockObserver->update();

    ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
    ASSERT_EQ(blockObserver->getStrides()[0], expectedObservationStride[0]);
    ASSERT_EQ(blockObserver->getStrides()[1], expectedObservationStride[1]);

    ASSERT_THAT(expectedImageData.get(), ObservationResultMatcher(blockObserver->getShape(), blockObserver->getStrides(), &updateObservation));
  }

  size_t dataLength = 4 * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  testEnvironment.verifyAndClearExpectations();
}

}  // namespace griddly
