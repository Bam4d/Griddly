#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/SpriteObserver.hpp"
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

void sprites_mockGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr, std::shared_ptr<MockObject>& mockAvatarObjectPtr) {
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

  // So the tiling for sprites is calculated correctly for walls
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{0, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{1, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{2, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{3, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{4, 0}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{0, 1}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{1, 1}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{2, 1}))).WillByDefault(Return(nullptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{3, 1}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{4, 1}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{0, 2}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{1, 2}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{2, 2}))).WillByDefault(Return(mockAvatarObjectPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{3, 2}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{4, 2}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{0, 3}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{1, 3}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{2, 3}))).WillByDefault(Return(nullptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{3, 3}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{4, 3}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{0, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{1, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{2, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{3, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(GridLocation{4, 4}))).WillByDefault(Return(mockObject1Ptr));

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

std::unordered_map<std::string, SpriteDefinition> getMockSpriteDefinitions() {
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
      {"avatar", mockAvatarSpriteDefinition},
      {"mo1", mockObject1SpriteDefinition},
      {"mo2", mockObject2SpriteDefinition},
      {"mo3", mockObject3SpriteDefinition},
  };
}

void runSpriteObserverTest(ObserverConfig observerConfig,
                           Direction avatarDirection,
                           std::vector<uint32_t> expectedObservationShape,
                           std::vector<uint32_t> expectedObservationStride,
                           std::string filenameExpectedOutputFilename,
                           bool trackAvatar,
                           bool writeOutputFile = false) {
  VulkanObserverConfig testConfig = {
      24,
      "resources/images",
      "resources/shaders"};
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<SpriteObserver> spriteObserver = std::shared_ptr<SpriteObserver>(new SpriteObserver(mockGridPtr, testConfig, getMockSpriteDefinitions()));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(avatarDirection));

  sprites_mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  spriteObserver->init(observerConfig);

  ASSERT_EQ(spriteObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(spriteObserver->getStrides(), expectedObservationStride);
  if (trackAvatar) {
    spriteObserver->setAvatar(mockAvatarObjectPtr);
  }
  auto resetObservation = spriteObserver->reset();
  auto updateObservation = spriteObserver->update(0);

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), spriteObserver->getStrides()[2], spriteObserver->getShape()[1], spriteObserver->getShape()[2]);
  }

  size_t dataLength = spriteObserver->getShape()[0] * spriteObserver->getShape()[1] * spriteObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(filenameExpectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
}

TEST(SpriteObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig.png", false);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};
  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig.png", true);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(SpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 120}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(SpriteObserverTest, partialObserver) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver.png", false);
}

TEST(SpriteObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset.png", false);
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_NONE.png", true);
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_UP.png", true);
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_RIGHT.png", true);
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_DOWN.png", true);
}

TEST(SpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_trackAvatar_LEFT.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_NONE.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_UP.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_RIGHT.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_DOWN.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_LEFT.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runSpriteObserverTest(config, Direction::NONE, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runSpriteObserverTest(config, Direction::UP, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runSpriteObserverTest(config, Direction::RIGHT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runSpriteObserverTest(config, Direction::DOWN, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(SpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runSpriteObserverTest(config, Direction::LEFT, {3, 120, 72}, {1, 3, 3 * 120}, "tests/resources/observer/sprite/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

}  // namespace griddly