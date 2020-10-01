#include "Griddly/Core/Observers/BlockObserver.hpp"
#include "VulkanObserverTest.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;

namespace griddly {

void blocks_mockRTSGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr) {
  // make a grid where multiple objects are owned by different players
  // 1  1   1   1   1
  // 1  A1  B2  C3  1
  // 1  A2  B3  C1  1
  // 1  A3  B1  C2  1
  // 1  1   1   1   1

  auto mockObjectWallPtr = mockObject(0, 3, "W");

  auto mockObjectA1Ptr = mockObject(1, 0, "A");
  auto mockObjectA2Ptr = mockObject(2, 0, "A");
  auto mockObjectA3Ptr = mockObject(3, 0, "A");

  auto mockObjectB1Ptr = mockObject(1, 1, "B");
  auto mockObjectB2Ptr = mockObject(2, 1, "B");
  auto mockObjectB3Ptr = mockObject(3, 1, "B");

  auto mockObjectC1Ptr = mockObject(1, 2, "C");
  auto mockObjectC2Ptr = mockObject(2, 2, "C");
  auto mockObjectC3Ptr = mockObject(3, 2, "C");

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB2Ptr}}));
  ;
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

  EXPECT_CALL(*mockGridPtr, getUniqueObjectCount).WillRepeatedly(Return(4));

  std::unordered_set<glm::ivec2> updatedLocations = {
      {0, 0},
      {0, 1},
      {0, 2},
      {0, 3},
      {0, 4},
      {1, 0},
      {1, 1},
      {1, 2},
      {1, 3},
      {1, 4},
      {2, 0},
      {2, 1},
      {2, 2},
      {2, 3},
      {2, 4},
      {3, 0},
      {3, 1},
      {3, 2},
      {3, 3},
      {3, 4},
      {4, 0},
      {4, 1},
      {4, 2},
      {4, 3},
      {4, 4},
  };

  ON_CALL(*mockGridPtr, getUpdatedLocations).WillByDefault(Return(updatedLocations));
}

std::unordered_map<std::string, BlockDefinition> getMockRTSBlockDefinitions() {
  float red[]{0.5, 0.2, 0.2};
  float green[]{0.2, 0.5, 0.2};
  float blue[]{0.2, 0.2, 0.5};
  float grey[]{0.3, 0.3, 0.3};

  std::string square = "square";
  std::string triangle = "triangle";

  // mock object 1
  BlockDefinition mockObjectABlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectABlockDefinition.color[c] = red[c];
  }
  mockObjectABlockDefinition.shape = square;
  mockObjectABlockDefinition.scale = 0.8f;
  mockObjectABlockDefinition.outlineScale = 4.0f;

  // mock object 2
  BlockDefinition mockObjectBBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectBBlockDefinition.color[c] = green[c];
  }
  mockObjectBBlockDefinition.shape = triangle;
  mockObjectBBlockDefinition.scale = 0.4f;
  mockObjectBBlockDefinition.outlineScale = 4.0f;

  // mock object 3
  BlockDefinition mockObjectCBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectCBlockDefinition.color[c] = blue[c];
  }
  mockObjectCBlockDefinition.shape = square;
  mockObjectCBlockDefinition.scale = 0.7f;
  mockObjectCBlockDefinition.outlineScale = 4.0f;

  // mock avatar 3
  BlockDefinition mockObjectWallBlockDefinition;
  for (std::size_t c = 0; c < 3; c++) {
    mockObjectWallBlockDefinition.color[c] = grey[c];
  }
  mockObjectWallBlockDefinition.shape = square;
  mockObjectWallBlockDefinition.scale = 1.0f;

  return {
      {"W0", mockObjectWallBlockDefinition},
      {"A0", mockObjectABlockDefinition},
      {"B0", mockObjectBBlockDefinition},
      {"C0", mockObjectCBlockDefinition},
  };
}

void blocks_mockGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr, std::shared_ptr<MockObject>& mockAvatarObjectPtr) {
  // make a 5 by 5 grid with an avatar in the center and some stuff around it, there are 4 types of object
  // "4" is the avatar type
  // 11111
  // 12031
  // 12431
  // 13021
  // 11111

  auto mockObject1Ptr = mockObject(1, 0, "mo1");
  auto mockObject2Ptr = mockObject(1, 1, "mo2");
  auto mockObject3Ptr = mockObject(1, 2, "mo3");

  EXPECT_CALL(*mockAvatarObjectPtr, getObjectId()).WillRepeatedly(Return(3));
  EXPECT_CALL(*mockAvatarObjectPtr, getLocation()).WillRepeatedly(Return(glm::ivec2{2, 2}));
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectName()).WillRepeatedly(Return("avatar"));
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectRenderTileName()).WillRepeatedly(Return("avatar" + std::to_string(0)));

  EXPECT_CALL(*mockGridPtr, getUniqueObjectCount).WillRepeatedly(Return(4));

  // A horrible way of making a mock grid but fuck it
  ON_CALL(*mockGridPtr, getObjectsAt).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockAvatarObjectPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  std::unordered_set<glm::ivec2> updatedLocations = {
      {0, 0},
      {0, 1},
      {0, 2},
      {0, 3},
      {0, 4},
      {1, 0},
      {1, 1},
      {1, 2},
      {1, 3},
      {1, 4},
      {2, 0},
      {2, 1},
      {2, 2},
      {2, 3},
      {2, 4},
      {3, 0},
      {3, 1},
      {3, 2},
      {3, 3},
      {3, 4},
      {4, 0},
      {4, 1},
      {4, 2},
      {4, 3},
      {4, 4},
  };

  ON_CALL(*mockGridPtr, getUpdatedLocations).WillByDefault(Return(updatedLocations));
}

std::unordered_map<std::string, BlockDefinition> getMockBlockDefinitions() {
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

  // mock avatar 3
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

void runBlockObserverTest(ObserverConfig observerConfig,
                          Direction avatarDirection,
                          std::vector<uint32_t> expectedObservationShape,
                          std::vector<uint32_t> expectedObservationStride,
                          std::string filenameExpectedOutputFilename,
                          bool trackAvatar,
                          bool writeOutputFile = false) {
  ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};

  observerConfig.tileSize = glm::ivec2(20, 20);

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(mockGridPtr, resourceConfig, getMockBlockDefinitions()));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(5));
  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(5));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto orientation = DiscreteOrientation(avatarDirection);
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(orientation));

  blocks_mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  blockObserver->init(observerConfig);

  if (trackAvatar) {
    blockObserver->setAvatar(mockAvatarObjectPtr);
  }

  auto resetObservation = blockObserver->reset();

  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides(), expectedObservationStride);

  auto updateObservation = blockObserver->update();

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), blockObserver->getStrides()[2], blockObserver->getShape()[1], blockObserver->getShape()[2]);
  }

  size_t dataLength = blockObserver->getShape()[0] * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(filenameExpectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatarObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

void runBlockObserverRTSTest(ObserverConfig observerConfig,
                             std::vector<uint32_t> expectedObservationShape,
                             std::vector<uint32_t> expectedObservationStride,
                             std::string expectedOutputFilename,
                             bool writeOutputFile = false) {
  ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};
  observerConfig.tileSize = glm::ivec2(50,50);

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(mockGridPtr, resourceConfig, getMockRTSBlockDefinitions()));

  blocks_mockRTSGridFunctions(mockGridPtr);

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(5));
  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(5));

  blockObserver->init(observerConfig);

  auto resetObservation = blockObserver->reset();

  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides(), expectedObservationStride);

  auto updateObservation = blockObserver->update();

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), blockObserver->getStrides()[2], blockObserver->getShape()[1], blockObserver->getShape()[2]);
  }

  size_t dataLength = blockObserver->getShape()[0] * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(BlockObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig.png", false);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};
  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(BlockObserverTest, partialObserver) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver.png", false);
}

TEST(BlockObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset.png", false);
}

TEST(BlockObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_RIGHT.png", true);
}

TEST(BlockObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_DOWN.png", true);
}

TEST(BlockObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_trackAvatar_LEFT.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_RIGHT.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_DOWN.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_LEFT.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runBlockObserverTest(config, Direction::NONE, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runBlockObserverTest(config, Direction::UP, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(BlockObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runBlockObserverTest(config, Direction::LEFT, {3, 100, 60}, {1, 3, 3 * 100}, "tests/resources/observer/block/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(BlockObserverTest, multiPlayer_Outline_Player1) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 250, 250}, {1, 3, 3 * 250}, "tests/resources/observer/block/multiPlayer_Outline_Player1.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Player2) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 250, 250}, {1, 3, 3 * 250}, "tests/resources/observer/block/multiPlayer_Outline_Player2.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Player3) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 250, 250}, {1, 3, 3 * 250}, "tests/resources/observer/block/multiPlayer_Outline_Player3.png");
}

TEST(BlockObserverTest, multiPlayer_Outline_Global) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  runBlockObserverRTSTest(config, {3, 250, 250}, {1, 3, 3 * 250}, "tests/resources/observer/block/multiPlayer_Outline_Global.png");
}

}  // namespace griddly