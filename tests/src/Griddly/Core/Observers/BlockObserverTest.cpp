#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/BlockObserver.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
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

void blocks_mockGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr, std::shared_ptr<MockObject>& mockAvatarObjectPtr) {
  // make a 5 by 5 grid with an avatar in the center and some stuff around it, there are 4 types of object
  // "4" is the avatar type
  // 11111
  // 12031
  // 12431
  // 13021
  // 11111

  auto mockObject1Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject1Ptr, getObjectId()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mockObject1Ptr, getObjectName()).WillRepeatedly(Return("mo1"));
  auto mockObject2Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject2Ptr, getObjectId()).WillRepeatedly(Return(1));
  EXPECT_CALL(*mockObject2Ptr, getObjectName()).WillRepeatedly(Return("mo2"));
  auto mockObject3Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject3Ptr, getObjectId()).WillRepeatedly(Return(2));
  EXPECT_CALL(*mockObject3Ptr, getObjectName()).WillRepeatedly(Return("mo3"));

  EXPECT_CALL(*mockAvatarObjectPtr, getObjectId()).WillRepeatedly(Return(3));
  EXPECT_CALL(*mockAvatarObjectPtr, getLocation()).WillRepeatedly(Return(GridLocation{2, 2}));
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectName()).WillRepeatedly(Return("avatar"));

  EXPECT_CALL(*mockGridPtr, getUniqueObjectCount).WillRepeatedly(Return(4));

  // A horrible way of making a mock grid but fuck it
  ON_CALL(*mockGridPtr, getObjectsAt).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{0, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{1, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{2, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{3, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{4, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{0, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{1, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{2, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{3, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{4, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{0, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{1, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{2, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockAvatarObjectPtr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{3, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{4, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{0, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{1, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject3Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{2, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{3, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{4, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{0, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{1, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{2, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{3, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(GridLocation{4, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  std::unordered_set<GridLocation, GridLocation::Hash> updatedLocations = {
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
      {"avatar", mockAvatarBlockDefinition},
      {"mo1", mockObject1BlockDefinition},
      {"mo2", mockObject2BlockDefinition},
      {"mo3", mockObject3BlockDefinition},
  };
}

void runBlockObserverTest(ObserverConfig observerConfig,
                          Direction avatarDirection,
                          std::vector<uint32_t> expectedObservationShape,
                          std::vector<uint32_t> expectedObservationStride,
                          std::string filenameExpectedOutputFilename,
                          bool trackAvatar,
                          bool writeOutputFile = false) {
  VulkanObserverConfig testConfig = {
      20,
      "resources/"};
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(mockGridPtr, testConfig, getMockBlockDefinitions()));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(avatarDirection));

  blocks_mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  blockObserver->init(observerConfig);

  ASSERT_EQ(blockObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(blockObserver->getStrides(), expectedObservationStride);
  if (trackAvatar) {
    blockObserver->setAvatar(mockAvatarObjectPtr);
  }
  auto resetObservation = blockObserver->reset();
  auto updateObservation = blockObserver->update(0);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), blockObserver->getStrides()[2], blockObserver->getShape()[0], blockObserver->getShape()[1]);
  }

  size_t dataLength = blockObserver->getShape()[0] * blockObserver->getShape()[1] * blockObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(filenameExpectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
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
      false};

  runBlockObserverTest(config, Direction::RIGHT, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runBlockObserverTest(config, Direction::DOWN, {3, 100, 100}, {1, 3, 3 * 100}, "tests/resources/observer/block/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(BlockObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

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

}  // namespace griddly