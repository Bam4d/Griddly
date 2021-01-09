#include "Griddly/Core/TestUtils/common.hpp"
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

class ObserverTestData {
 public:
  ObserverTestData(DiscreteOrientation orientation) {
    mockAvatarObjectPtr = mockObject("avatar", 1, 3, 0, {2, 2});
    mockSinglePlayerObject1Ptr = mockObject("mo1", 1, 0);
    mockSinglePlayerObject2Ptr = mockObject("mo2", 1, 1);
    mockSinglePlayerObject3Ptr = mockObject("mo3", 1, 2);
    mockSinglePlayerObjects = {mockSinglePlayerObject1Ptr, mockSinglePlayerObject2Ptr, mockSinglePlayerObject3Ptr};

    mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
    EXPECT_CALL(*mockGridPtr, getWidth)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*mockGridPtr, getHeight)
        .WillRepeatedly(Return(5));

    EXPECT_CALL(*mockGridPtr, getObjects()).WillRepeatedly(ReturnRef(mockSinglePlayerObjects));
    EXPECT_CALL(*mockGridPtr, getUniqueObjectCount).WillRepeatedly(Return(4));
    EXPECT_CALL(*mockGridPtr, getUpdatedLocations).WillRepeatedly(ReturnRef(mockSinglePlayerUpdatedLocations));
    EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const TileObjects& {
      return mockSinglePlayerGridData.at(location);
    }));

    EXPECT_CALL(*mockAvatarObjectPtr, getObjectRenderTileName()).WillRepeatedly(Return("avatar" + std::to_string(0)));
    EXPECT_CALL(*mockAvatarObjectPtr, getObjectOrientation).WillRepeatedly(Return(orientation));
  }

  std::shared_ptr<MockGrid> mockGridPtr;

  std::shared_ptr<MockObject> mockAvatarObjectPtr;
  std::shared_ptr<MockObject> mockSinglePlayerObject1Ptr;
  std::shared_ptr<MockObject> mockSinglePlayerObject2Ptr;
  std::shared_ptr<MockObject> mockSinglePlayerObject3Ptr;

  std::unordered_set<std::shared_ptr<Object>> mockSinglePlayerObjects;

  const std::unordered_map<glm::ivec2, TileObjects> mockSinglePlayerGridData = {
      {{0, 0}, {{0, mockSinglePlayerObject1Ptr}}},
      {{1, 0}, {{0, mockSinglePlayerObject1Ptr}}},
      {{2, 0}, {{0, mockSinglePlayerObject1Ptr}}},
      {{3, 0}, {{0, mockSinglePlayerObject1Ptr}}},
      {{4, 0}, {{0, mockSinglePlayerObject1Ptr}}},

      {{0, 1}, {{0, mockSinglePlayerObject1Ptr}}},
      {{1, 1}, {{0, mockSinglePlayerObject2Ptr}}},
      {{2, 1}, {{}}},
      {{3, 1}, {{0, mockSinglePlayerObject3Ptr}}},
      {{4, 1}, {{0, mockSinglePlayerObject1Ptr}}},

      {{0, 2}, {{0, mockSinglePlayerObject1Ptr}}},
      {{1, 2}, {{0, mockSinglePlayerObject2Ptr}}},
      {{2, 2}, {{0, mockAvatarObjectPtr}}},
      {{3, 2}, {{0, mockSinglePlayerObject3Ptr}}},
      {{4, 2}, {{0, mockSinglePlayerObject1Ptr}}},

      {{0, 3}, {{0, mockSinglePlayerObject1Ptr}}},
      {{1, 3}, {{0, mockSinglePlayerObject3Ptr}}},
      {{2, 3}, {{}}},
      {{3, 3}, {{0, mockSinglePlayerObject2Ptr}}},
      {{4, 3}, {{0, mockSinglePlayerObject1Ptr}}},

      {{0, 4}, {{0, mockSinglePlayerObject1Ptr}}},
      {{1, 4}, {{0, mockSinglePlayerObject1Ptr}}},
      {{2, 4}, {{0, mockSinglePlayerObject1Ptr}}},
      {{3, 4}, {{0, mockSinglePlayerObject1Ptr}}},
      {{4, 4}, {{0, mockSinglePlayerObject1Ptr}}},
  };

  const std::unordered_set<glm::ivec2> mockSinglePlayerUpdatedLocations = {
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

  void verifyAndClearExpectations() {
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatarObjectPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  }
};

}  // namespace griddly