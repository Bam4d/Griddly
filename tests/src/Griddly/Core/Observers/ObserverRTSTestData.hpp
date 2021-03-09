#include "Griddly/Core/TestUtils/common.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

#define _V(X) std::make_shared<int32_t>(X)

class ObserverRTSTestData {
 public:
  ObserverRTSTestData(ObserverConfig observerConfig) {
    // make a grid where multiple objects are owned by different players
    // 1  1   1   1   1
    // 1  A1  B2  C3  1
    // 1  A2  B3  C1  1
    // 1  A3  B1  C2  1
    // 1  1   1   1   1

    if(observerConfig.includeVariables) {
      mockObjectWallPtr = mockObject("W", 0, 0, {0,0}, DiscreteOrientation(), {}, {});
      mockObjectA1Ptr = mockObject("A", 1, 0, {0,0}, DiscreteOrientation(), {}, {{"V1", _V(1)}, {"_ignored", _V(10)}});
      mockObjectA2Ptr = mockObject("A", 2, 0, {0,0}, DiscreteOrientation(), {}, {{"V2", _V(2)}, {"_ignored", _V(10)}});
      mockObjectA3Ptr = mockObject("A", 3, 0, {0,0}, DiscreteOrientation(), {}, {{"V3", _V(3)}, {"_ignored", _V(10)}});
      mockObjectB1Ptr = mockObject("B", 1, 0, {0,0}, DiscreteOrientation(), {}, {{"V1", _V(4)}, {"_ignored", _V(10)}});
      mockObjectB2Ptr = mockObject("B", 2, 0, {0,0}, DiscreteOrientation(), {}, {{"V2", _V(5)}, {"_ignored", _V(10)}});
      mockObjectB3Ptr = mockObject("B", 3, 0, {0,0}, DiscreteOrientation(), {}, {{"V3", _V(6)}, {"_ignored", _V(10)}});
      mockObjectC1Ptr = mockObject("C", 1, 0, {0,0}, DiscreteOrientation(), {}, {{"V1", _V(7)}, {"_ignored", _V(10)}});
      mockObjectC2Ptr = mockObject("C", 2, 0, {0,0}, DiscreteOrientation(), {}, {{"V2", _V(8)}, {"_ignored", _V(10)}});
      mockObjectC3Ptr = mockObject("C", 3, 0, {0,0}, DiscreteOrientation(), {}, {{"V3", _V(9)}, {"_ignored", _V(10)}});
    } else {
      mockObjectWallPtr = mockObject("W", 0);
      mockObjectA1Ptr = mockObject("A", 1);
      mockObjectA2Ptr = mockObject("A", 2);
      mockObjectA3Ptr = mockObject("A", 3);
      mockObjectB1Ptr = mockObject("B", 1);
      mockObjectB2Ptr = mockObject("B", 2);
      mockObjectB3Ptr = mockObject("B", 3);
      mockObjectC1Ptr = mockObject("C", 1);
      mockObjectC2Ptr = mockObject("C", 2);
      mockObjectC3Ptr = mockObject("C", 3);
    }

    

    mockRTSObjects = std::unordered_set<std::shared_ptr<Object>>{
        mockObjectWallPtr,
        mockObjectA1Ptr,
        mockObjectA2Ptr,
        mockObjectA3Ptr,
        mockObjectB1Ptr,
        mockObjectB2Ptr,
        mockObjectB3Ptr,
        mockObjectC1Ptr,
        mockObjectC2Ptr,
        mockObjectC3Ptr};

    mockRTSGridData = {
        {{0, 0}, {{0, mockObjectWallPtr}}},
        {{1, 0}, {{0, mockObjectWallPtr}}},
        {{2, 0}, {{0, mockObjectWallPtr}}},
        {{3, 0}, {{0, mockObjectWallPtr}}},
        {{4, 0}, {{0, mockObjectWallPtr}}},

        {{0, 1}, {{0, mockObjectWallPtr}}},
        {{1, 1}, {{0, mockObjectA1Ptr}}},
        {{2, 1}, {{0, mockObjectB1Ptr}}},
        {{3, 1}, {{0, mockObjectC1Ptr}}},
        {{4, 1}, {{0, mockObjectWallPtr}}},

        {{0, 2}, {{0, mockObjectWallPtr}}},
        {{1, 2}, {{0, mockObjectA2Ptr}}},
        {{2, 2}, {{0, mockObjectB2Ptr}}},
        {{3, 2}, {{0, mockObjectC2Ptr}}},
        {{4, 2}, {{0, mockObjectWallPtr}}},

        {{0, 3}, {{0, mockObjectWallPtr}}},
        {{1, 3}, {{0, mockObjectA3Ptr}}},
        {{2, 3}, {{0, mockObjectB3Ptr}}},
        {{3, 3}, {{0, mockObjectC3Ptr}}},
        {{4, 3}, {{0, mockObjectWallPtr}}},

        {{0, 4}, {{0, mockObjectWallPtr}}},
        {{1, 4}, {{0, mockObjectWallPtr}}},
        {{2, 4}, {{0, mockObjectWallPtr}}},
        {{3, 4}, {{0, mockObjectWallPtr}}},
        {{4, 4}, {{0, mockObjectWallPtr}}},
    };

    mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
    EXPECT_CALL(*mockGridPtr, getWidth)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*mockGridPtr, getHeight)
        .WillRepeatedly(Return(5));

    EXPECT_CALL(*mockGridPtr, getObjects()).WillRepeatedly(ReturnRef(mockRTSObjects));
    EXPECT_CALL(*mockGridPtr, getUpdatedLocations).WillRepeatedly(ReturnRef(mockRTSUpdatedLocations));

    EXPECT_CALL(*mockGridPtr, getObjectVariableIds()).WillRepeatedly(ReturnRef(mockObjectVariableIds));
    EXPECT_CALL(*mockGridPtr, getObjectIds()).WillRepeatedly(ReturnRef(mockSinglePlayerObjectIds));

    bool hasOffsets = observerConfig.gridXOffset != 0 || observerConfig.gridYOffset != 0;

    if (!hasOffsets) {
      EXPECT_CALL(*mockGridPtr, purgeUpdatedLocations).Times(AtLeast(1));
    }

    EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const TileObjects& {
      return mockRTSGridData.at(location);
    }));

    EXPECT_CALL(*mockGridPtr, getObject).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const std::shared_ptr<Object> {
      if (mockRTSGridData.find(location) == mockRTSGridData.end()) {
        return nullptr;
      }

      auto objectsAt = mockRTSGridData.at(location);
      if (objectsAt.size() > 0) {
        return objectsAt.at(0);
      } else {
        return nullptr;
      }
    }));

  }

  std::shared_ptr<MockGrid> mockGridPtr;

  const std::unordered_map<std::string, uint32_t> mockSinglePlayerObjectIds = {
    {"W", 3},
    {"A", 0},
    {"B", 1},
    {"C", 2}
  };

  const std::unordered_map<std::string, uint32_t> mockObjectVariableIds = {
    {"V1", 0},
    {"V2", 1},
    {"V3", 2}
  };

  std::shared_ptr<MockObject> mockObjectWallPtr;
  std::shared_ptr<MockObject> mockObjectA1Ptr;
  std::shared_ptr<MockObject> mockObjectA2Ptr;
  std::shared_ptr<MockObject> mockObjectA3Ptr;
  std::shared_ptr<MockObject> mockObjectB1Ptr;
  std::shared_ptr<MockObject> mockObjectB2Ptr;
  std::shared_ptr<MockObject> mockObjectB3Ptr;
  std::shared_ptr<MockObject> mockObjectC1Ptr;
  std::shared_ptr<MockObject> mockObjectC2Ptr;
  std::shared_ptr<MockObject> mockObjectC3Ptr;

  std::unordered_set<std::shared_ptr<Object>> mockRTSObjects;
  std::unordered_map<glm::ivec2, TileObjects> mockRTSGridData;

  const std::unordered_set<glm::ivec2> mockRTSUpdatedLocations = {
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
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  }
};

}