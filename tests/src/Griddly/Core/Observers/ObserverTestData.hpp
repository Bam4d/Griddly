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
using ::testing::AtLeast;

namespace griddly {

class ObserverTestData {
 public:
  ObserverTestData(ObserverConfig observerConfig, DiscreteOrientation orientation, bool trackAvatar) {
    mockAvatarObjectPtr = mockObject("avatar", 'A', 1, 0, {2, 2});
    mockSinglePlayerObject1Ptr = mockObject("mo1", 'W', 1);
    mockSinglePlayerObject2Ptr = mockObject("mo2", 'P', 1);
    mockSinglePlayerObject3Ptr = mockObject("mo3", 'Q', 1);
    mockSinglePlayerObjects = {mockSinglePlayerObject1Ptr, mockSinglePlayerObject2Ptr, mockSinglePlayerObject3Ptr};

    mockSinglePlayerGridData = {
        {{0, 0}, {{0, mockSinglePlayerObject1Ptr}}},
        {{1, 0}, {{0, mockSinglePlayerObject1Ptr}}},
        {{2, 0}, {{0, mockSinglePlayerObject1Ptr}}},
        {{3, 0}, {{0, mockSinglePlayerObject1Ptr}}},
        {{4, 0}, {{0, mockSinglePlayerObject1Ptr}}},

        {{0, 1}, {{0, mockSinglePlayerObject1Ptr}}},
        {{1, 1}, {{0, mockSinglePlayerObject2Ptr}}},
        {{2, 1}, {}},
        {{3, 1}, {{0, mockSinglePlayerObject3Ptr}}},
        {{4, 1}, {{0, mockSinglePlayerObject1Ptr}}},

        {{0, 2}, {{0, mockSinglePlayerObject1Ptr}}},
        {{1, 2}, {{0, mockSinglePlayerObject2Ptr}}},
        {{2, 2}, {{0, mockAvatarObjectPtr}}},
        {{3, 2}, {{0, mockSinglePlayerObject3Ptr}}},
        {{4, 2}, {{0, mockSinglePlayerObject1Ptr}}},

        {{0, 3}, {{0, mockSinglePlayerObject1Ptr}}},
        {{1, 3}, {{0, mockSinglePlayerObject3Ptr}}},
        {{2, 3}, {}},
        {{3, 3}, {{0, mockSinglePlayerObject2Ptr}}},
        {{4, 3}, {{0, mockSinglePlayerObject1Ptr}}},

        {{0, 4}, {{0, mockSinglePlayerObject1Ptr}}},
        {{1, 4}, {{0, mockSinglePlayerObject1Ptr}}},
        {{2, 4}, {{0, mockSinglePlayerObject1Ptr}}},
        {{3, 4}, {{0, mockSinglePlayerObject1Ptr}}},
        {{4, 4}, {{0, mockSinglePlayerObject1Ptr}}},
    };

    mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
    EXPECT_CALL(*mockGridPtr, getWidth)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*mockGridPtr, getHeight)
        .WillRepeatedly(Return(5));

    EXPECT_CALL(*mockGridPtr, getObjects()).WillRepeatedly(ReturnRef(mockSinglePlayerObjects));
    EXPECT_CALL(*mockGridPtr, getUpdatedLocations).WillRepeatedly(ReturnRef(mockSinglePlayerUpdatedLocations));
    EXPECT_CALL(*mockGridPtr, getObjectIds()).WillRepeatedly(ReturnRef(mockSinglePlayerObjectIds));

    bool hasOffsets = observerConfig.gridXOffset != 0 || observerConfig.gridYOffset != 0;

    if (!trackAvatar && !hasOffsets) {
      EXPECT_CALL(*mockGridPtr, purgeUpdatedLocations).Times(AtLeast(1));
    }
    EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const TileObjects& {
      return mockSinglePlayerGridData.at(location);
    }));

    EXPECT_CALL(*mockGridPtr, getObject).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const std::shared_ptr<Object> {

      if(mockSinglePlayerGridData.find(location) == mockSinglePlayerGridData.end()) {
        return nullptr;
      }

      auto objectsAt = mockSinglePlayerGridData.at(location);
      if (objectsAt.size() > 0) {
        return objectsAt.at(0);
      } else {
        return nullptr;
      }
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
  std::unordered_map<glm::ivec2, TileObjects> mockSinglePlayerGridData;

  const std::unordered_map<std::string, uint32_t> mockSinglePlayerObjectIds = {
    {"avatar", 3},
    {"mo1", 0},
    {"mo2", 1},
    {"mo3", 2}
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