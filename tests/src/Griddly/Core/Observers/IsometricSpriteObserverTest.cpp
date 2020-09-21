#include "Griddly/Core/Observers/IsometricSpriteObserver.hpp"
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

void isometricSprites_mockRTSGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr) {
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

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(mockObjectWallPtr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(mockObjectA1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(mockObjectB1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(mockObjectC1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(mockObjectWallPtr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(mockObjectA2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(mockObjectB2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(mockObjectC2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(mockObjectWallPtr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(mockObjectA3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(mockObjectB3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(mockObjectC3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(mockObjectWallPtr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(mockObjectWallPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(mockObjectWallPtr));

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

std::unordered_map<std::string, SpriteDefinition> getMockRTSIsometricSpriteDefinitions() {
  // mock wall object
  SpriteDefinition mockObject1SpriteDefinition;
  mockObject1SpriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
  mockObject1SpriteDefinition.offset = glm::ivec2(0, 3);
  mockObject1SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/lava.png",
  };

  // mock object 2
  SpriteDefinition mockObject2SpriteDefinition;
  mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject2SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/crate.png",
  };

  // mock object 3
  SpriteDefinition mockObject3SpriteDefinition;
  mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject3SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/bush.png",
  };

  // mock avatar 3
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/avatars/spider1.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/grass.png",
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
  observerConfig.isoTileYOffset = 16;
  
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(mockGridPtr, resourceConfig, getMockRTSIsometricSpriteDefinitions()));

  isometricSprites_mockRTSGridFunctions(mockGridPtr);

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(5));
  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(5));

  isometricObserver->init(observerConfig);

  auto resetObservation = isometricObserver->reset();

  ASSERT_EQ(isometricObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(isometricObserver->getStrides(), expectedObservationStride);

  auto updateObservation = isometricObserver->update();

  if (writeOutputFile) {
    std::string testName(::testing::UnitTest::GetInstance()->current_test_info()->name());
    write_image(testName + ".png", resetObservation.get(), isometricObserver->getStrides()[2], isometricObserver->getShape()[1], isometricObserver->getShape()[2]);
  }

  size_t dataLength = isometricObserver->getShape()[0] * isometricObserver->getShape()[1] * isometricObserver->getShape()[2];

  auto expectedImageData = loadExpectedImage(expectedOutputFilename);

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedImageData.get(), dataLength));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

void isometricSprites_mockGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr, std::shared_ptr<MockObject>& mockAvatarObjectPtr) {
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
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject2Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));
  ON_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(std::map<uint32_t, std::shared_ptr<Object>>{{0, mockObject1Ptr}}));

  // So the tiling for isometrics is calculated correctly for walls
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 0}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 0}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 1}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 1}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 1}))).WillByDefault(Return(nullptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 1}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 1}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 2}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 2}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 2}))).WillByDefault(Return(mockAvatarObjectPtr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 2}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 2}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 3}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 3}))).WillByDefault(Return(mockObject3Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 3}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 3}))).WillByDefault(Return(mockObject2Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 3}))).WillByDefault(Return(mockObject1Ptr));

  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{0, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{1, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{2, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{3, 4}))).WillByDefault(Return(mockObject1Ptr));
  ON_CALL(*mockGridPtr, getObject(Eq(glm::ivec2{4, 4}))).WillByDefault(Return(mockObject1Ptr));

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

std::unordered_map<std::string, SpriteDefinition> getMockIsometricSpriteDefinitions() {
  // mock object 1
  SpriteDefinition mockObject1SpriteDefinition;
  mockObject1SpriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
  mockObject1SpriteDefinition.offset = glm::ivec2(0, 3);
  mockObject1SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/lava.png",
  };

  // mock object 2
  SpriteDefinition mockObject2SpriteDefinition;
  mockObject2SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject2SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/crate.png",
  };

  // mock object 3
  SpriteDefinition mockObject3SpriteDefinition;
  mockObject3SpriteDefinition.tilingMode = TilingMode::NONE;
  mockObject3SpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/bush.png",
  };

  // mock avatar 3
  SpriteDefinition mockAvatarSpriteDefinition;
  mockAvatarSpriteDefinition.tilingMode = TilingMode::NONE;
  mockAvatarSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/avatars/spider1.png",
  };

  // __background__
  SpriteDefinition backgroundSpriteDefinition;
  backgroundSpriteDefinition.tilingMode = TilingMode::NONE;
  backgroundSpriteDefinition.images = {
      "oryx/oryx_iso_dungeon/grass.png",
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
  observerConfig.isoTileYOffset = 16;

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<IsometricSpriteObserver> isometricObserver = std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(mockGridPtr, resourceConfig, getMockIsometricSpriteDefinitions()));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto orientation = DiscreteOrientation(avatarDirection);
  EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(orientation));

  isometricSprites_mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(5));
  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(5));

  isometricObserver->init(observerConfig);

  if (trackAvatar) {
    isometricObserver->setAvatar(mockAvatarObjectPtr);
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

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatarObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", false);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::UP, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset.png", false);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      false};
  
  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::NONE, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::UP, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::RIGHT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::DOWN, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN.png", true);
}

TEST(IsometricSpriteObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};
  
  runIsometricSpriteObserverTest(config, Direction::LEFT, {3, 128, 96}, {1, 3, 3 * 128}, "tests/resources/observer/isometric/partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT.png", true);
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player1) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;
  
  runIsometricSpriteObserverRTSTest(config, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player1.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player2) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;
  
  runIsometricSpriteObserverRTSTest(config, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player2.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Player3) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;
  
  runIsometricSpriteObserverRTSTest(config, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Player3.png");
}

TEST(IsometricSpriteObserverTest, multiPlayer_Outline_Global) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;
  
  runIsometricSpriteObserverRTSTest(config, {3, 160, 112}, {1, 3, 3 * 160}, "tests/resources/observer/isometric/multiPlayer_Outline_Global.png");
}

}  // namespace griddly