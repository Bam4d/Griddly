#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/VectorObserver.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;

namespace griddly {

void mockGridFunctions(std::shared_ptr<MockGrid>& mockGridPtr, std::shared_ptr<MockObject>& mockAvatarObjectPtr) {
  // make a 5 by 5 grid with an avatar in the center and some stuff around it, there are 4 types of object
  // "4" is the avatar type
  // 11111
  // 12031
  // 12431
  // 13021
  // 11111

  auto mockObject1Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject1Ptr, getObjectId()).WillRepeatedly(Return(0));
  auto mockObject2Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject2Ptr, getObjectId()).WillRepeatedly(Return(1));
  auto mockObject3Ptr = std::shared_ptr<MockObject>(new MockObject());
  EXPECT_CALL(*mockObject3Ptr, getObjectId()).WillRepeatedly(Return(2));

  EXPECT_CALL(*mockAvatarObjectPtr, getObjectId()).WillRepeatedly(Return(3));

  EXPECT_CALL(*mockAvatarObjectPtr, getLocation()).WillRepeatedly(Return(GridLocation{2, 2}));

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
}

TEST(VectorObserverTest, defaultObserverConfig) {
}

TEST(VectorObserverTest, partialObserver) {
}

TEST(VectorObserverTest, partialObserverTrackingAvatar) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<VectorObserver> vectorObserver = std::shared_ptr<VectorObserver>(new VectorObserver(mockGridPtr));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  ObserverConfig config = {
      3,
      5,
      0,
      0,
      false};

  // Observer should be able to see
  // 111
  // 203
  // 243
  // 302
  // 111

  vectorObserver->init(config);
  vectorObserver->setAvatar(mockAvatarObjectPtr);
  auto resetObservation = vectorObserver->reset();
  auto updateObservation = vectorObserver->update(0);

  size_t dataLength = 4 * 3 * 5;

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}},
      {{0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
}

TEST(VectorObserverTest, partialObserverTrackingAvatarWithOffset) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<VectorObserver> vectorObserver = std::shared_ptr<VectorObserver>(new VectorObserver(mockGridPtr));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  ObserverConfig config = {
      3,
      5,
      -1,
      -2,
      false};

  // Observer should be able to see
  // 124
  // 130
  // 111
  // 000
  // 000

  vectorObserver->init(config);
  vectorObserver->setAvatar(mockAvatarObjectPtr);
  auto resetObservation = vectorObserver->reset();
  auto updateObservation = vectorObserver->update(0);

  size_t dataLength = 4 * 3 * 5;

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
}

TEST(VectorObserverTest, partialObserverTrackingAvatarAndRotationAndOffset) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<VectorObserver> vectorObserver = std::shared_ptr<VectorObserver>(new VectorObserver(mockGridPtr));

  auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  mockGridFunctions(mockGridPtr, mockAvatarObjectPtr);

  EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(Direction::RIGHT));

  ObserverConfig config = {
      3,
      5,
      0,
      2,
      true,
  };

  // Observer should be able to see
  // 000
  // 000
  // 111
  // 332
  // 040

  vectorObserver->init(config);
  vectorObserver->setAvatar(mockAvatarObjectPtr);
  auto resetObservation = vectorObserver->reset();
  auto updateObservation = vectorObserver->update(0);

  size_t dataLength = 4 * 3 * 5;

  uint8_t expectedData[5][3][4] = {
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 1}, {1, 0, 0, 0}},
  };

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation.get(), resetObservation.get() + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation.get(), updateObservation.get() + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData[0][0], dataLength));
}
}  // namespace griddly