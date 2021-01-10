#include "Griddly/Core/Observers/IsometricSpriteObserver.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
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

// std::unordered_set<std::shared_ptr<Object>> isometricSprites_mockRTSGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr) {
//   // make a grid where multiple objects are owned by different players
//   // 1  1   1   1   1
//   // 1  A1  B2  C3  1
//   // 1  A2  B3  C1  1
//   // 1  A3  B1  C2  1
//   // 1  1   1   1   1

//   auto mockObjectWallPtr = mockObject("W", 0, 3);

//   auto mockObjectA1Ptr = mockObject("A", 1, 0);
//   auto mockObjectA2Ptr = mockObject("A", 2, 0);
//   auto mockObjectA3Ptr = mockObject("A", 3, 0);

//   auto mockObjectB1Ptr = mockObject("B", 1, 1);
//   auto mockObjectB2Ptr = mockObject("B", 2, 1);
//   auto mockObjectB3Ptr = mockObject("B", 3, 1);

//   auto mockObjectC1Ptr = mockObject("C", 1, 2);
//   auto mockObjectC2Ptr = mockObject("C", 2, 2);
//   auto mockObjectC3Ptr = mockObject("C", 3, 2);

//   auto objects = std::unordered_set<std::shared_ptr<Object>>{
//       mockObjectWallPtr,
//       mockObjectA1Ptr,
//       mockObjectA2Ptr,
//       mockObjectA3Ptr,
//       mockObjectB1Ptr,
//       mockObjectB2Ptr,
//       mockObjectB3Ptr,
//       mockObjectC1Ptr,
//       mockObjectC2Ptr,
//       mockObjectC3Ptr};

//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA1Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB1Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC1Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA2Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB2Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC2Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectA3Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectB3Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectC3Ptr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));
//   ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObjectWallPtr}}));

//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(mockObjectWallPtr));

//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(mockObjectA1Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(mockObjectB1Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(mockObjectC1Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(mockObjectWallPtr));

//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(mockObjectA2Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(mockObjectB2Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(mockObjectC2Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(mockObjectWallPtr));

//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(mockObjectA3Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(mockObjectB3Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(mockObjectC3Ptr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(mockObjectWallPtr));

//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(mockObjectWallPtr));
//   ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(mockObjectWallPtr));

//   EXPECT_CALL(*mockGridPtr, getUniqueObjectCount).WillRepeatedly(Return(4));

//   std::unordered_set<glm::ivec2> updatedLocations = {
//       {0, 0},
//       {0, 1},
//       {0, 2},
//       {0, 3},
//       {0, 4},
//       {1, 0},
//       {1, 1},
//       {1, 2},
//       {1, 3},
//       {1, 4},
//       {2, 0},
//       {2, 1},
//       {2, 2},
//       {2, 3},
//       {2, 4},
//       {3, 0},
//       {3, 1},
//       {3, 2},
//       {3, 3},
//       {3, 4},
//       {4, 0},
//       {4, 1},
//       {4, 2},
//       {4, 3},
//       {4, 4},
//   };

//   ON_CALL(*mockGridPtr, getUpdatedLocations).WillByDefault(Return(updatedLocations));

//   return objects;
// }

// std::unordered_map<std::string, SpriteDefinition> getMockRTSIsometricSpriteDefinitions() {
//   // mock wall object
//   SpriteDefinition mockObject1SpriteDefinition;
//   mockObject1SpriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
//   mockObject1SpriteDefinition.offset = glm::ivec2(0, 3);
//   mockObject1SpriteDefinition.images = {
//       "oryx/oryx_iso_dungeon/lava-1.png",
//   };

//   // mock object 2
//   SpriteDefinition mockObject2SpriteDefinition;
//   mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
//   mockObject2SpriteDefinition.images = {
//       "oryx/oryx_iso_dungeon/crate-1.png",
//   };

//   // mock object 3
//   SpriteDefinition mockObject3SpriteDefinition;
//   mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
//   mockObject3SpriteDefinition.images = {
//       "oryx/oryx_iso_dungeon/bush-1.png",
//   };

//   // mock avatar 3
//   SpriteDefinition mockAvatarSpriteDefinition;
//   mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
//   mockAvatarSpriteDefinition.images = {
//       "oryx/oryx_iso_dungeon/avatars/spider-1.png",
//   };

//   // __background__
//   SpriteDefinition backgroundSpriteDefinition;
//   backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
//   backgroundSpriteDefinition.images = {
//       "oryx/oryx_iso_dungeon/grass-1.png",
//   };

//   return {
//       {"_iso_background_", backgroundSpriteDefinition},
//       {"W0", mockObject1SpriteDefinition},
//       {"A0", mockAvatarSpriteDefinition},
//       {"B0", mockObject2SpriteDefinition},
//       {"C0", mockObject3SpriteDefinition},
//   };
// }

// void runIsometricSpriteObserverRTSTest(ObserverConfig observerConfig,
//                                        std::vector<uint32_t> expectedObservationShape,
//                                        std::vector<uint32_t> expectedObservationStride,
//                                        std::string expectedOutputFilename,
//                                        bool writeOutputFile = false) {
//   ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};

//   observerConfig.tileSize = glm::ivec2(32, 48);
//   observerConfig.isoTileHeight = 16;
//   observerConfig.isoTileDepth = 4;

//   auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
//   std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(mockGridPtr, resourceConfig, getMockRTSIsometricSpriteDefinitions()));

//   auto objects = isometricSprites_mockRTSGridFunctions(mockGridPtr);

//   EXPECT_CALL(*mockGridPtr, getObjects)
//       .WillRepeatedly(ReturnRef(objects));

//   EXPECT_CALL(*mockGridPtr, getWidth)
//       .WillRepeatedly(Return(5));
//   EXPECT_CALL(*mockGridPtr, getHeight)
//       .WillRepeatedly(Return(5));

//   isometricObserver->init(observerConfig);

//   auto resetObservation = isometricObserver->reset();

//   ASSERT_EQ(isometricObserver->getTileSize(), glm::ivec2(32, 48));
//   ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
//   ASSERT_EQ(isometricObserver->getStrides(), expectedObservationStride);

//   auto updateObservation = isometricObserver->update();

//   if (writeOutputFile) {
//     std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
//     write_image(testName + ".png", resetObservation.get(), isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
//   }

//   size_t dataLength = isometricObserver->getShape()[0] * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

//   auto expectedImageData = loadExpectedImage(expectedOutputFilename);

//   auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
//   auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

//   ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
//   ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));

//   EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
// }

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
                                    std::string filenameExpectedOutputFilename,
                                    bool trackAvatar,
                                    bool writeOutputFile = false) {
  ResourceConfig resourceConfig = {"resources/images", "resources/shaders"};

  observerConfig.tileSize = glm::ivec2(32, 48);
  observerConfig.isoTileHeight = 16;
  observerConfig.isoTileDepth = 4;

  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection), trackAvatar);

  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(testEnvironment.mockGridPtr, resourceConfig, getMockIsometricSpriteDefinitions()));


  isometricObserver->init(observerConfig);

  if (trackAvatar) {
    isometricObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }

  auto resetObservation = isometricObserver->reset();
  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides(), expectedObservationStride);
  auto updateObservation = isometricObserver->update();

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = isometricObserver->getShape()[0] * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(filenameExpectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));

  testEnvironment.verifyAndClearExpectations();
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", false, true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 116}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

// TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player1) {
//   ObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 1;
//   config.playerCount = 3;

//   runIsometricSpriteObserverRTSTest(config, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player1.png");
// }

// TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player2) {
//   ObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 2;
//   config.playerCount = 3;

//   runIsometricSpriteObserverRTSTest(config, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player2.png");
// }

// TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player3) {
//   ObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 3;
//   config.playerCount = 3;

//   runIsometricSpriteObserverRTSTest(config, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player3.png");
// }

// TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Global) {
//   ObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 0;
//   config.playerCount = 3;

//   runIsometricSpriteObserverRTSTest(config, {3, 160, 132}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Global.png");
// }

}  // namespace griddly