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

    std::vector<std::shared_ptr<MockObject>> walls;
    walls.push_back(mockObject("W", 'W', 0, 0, {0, 0}));
    walls.push_back(mockObject("W", 'W', 0, 0, {1, 0}));
    walls.push_back(mockObject("W", 'W', 0, 0, {2, 0}));
    walls.push_back(mockObject("W", 'W', 0, 0, {3, 0}));
    walls.push_back(mockObject("W", 'W', 0, 0, {4, 0}));
    walls.push_back(mockObject("W", 'W', 0, 0, {0, 1}));
    walls.push_back(mockObject("W", 'W', 0, 0, {4, 1}));
    walls.push_back(mockObject("W", 'W', 0, 0, {0, 2}));
    walls.push_back(mockObject("W", 'W', 0, 0, {4, 2}));
    walls.push_back(mockObject("W", 'W', 0, 0, {0, 3}));
    walls.push_back(mockObject("W", 'W', 0, 0, {4, 3}));
    walls.push_back(mockObject("W", 'W', 0, 0, {0, 4}));
    walls.push_back(mockObject("W", 'W', 0, 0, {1, 4}));
    walls.push_back(mockObject("W", 'W', 0, 0, {2, 4}));
    walls.push_back(mockObject("W", 'W', 0, 0, {3, 4}));
    walls.push_back(mockObject("W", 'W', 0, 0, {4, 4}));

    std::vector<std::shared_ptr<MockObject>> objectAs;
    std::vector<std::shared_ptr<MockObject>> objectBs;
    std::vector<std::shared_ptr<MockObject>> objectCs;

    objectAs.push_back(mockObject("A", 'A', 1, 0, {1, 1}, DiscreteOrientation(), {}, {{"V1", _V(1)}, {"_ignored", _V(10)}}));
    objectAs.push_back(mockObject("A", 'A', 2, 0, {1, 2}, DiscreteOrientation(), {}, {{"V2", _V(2)}, {"_ignored", _V(10)}}));
    objectAs.push_back(mockObject("A", 'A', 3, 0, {1, 3}, DiscreteOrientation(), {}, {{"V3", _V(3)}, {"_ignored", _V(10)}}));
    objectBs.push_back(mockObject("B", 'B', 1, 0, {2, 1}, DiscreteOrientation(), {}, {{"V1", _V(4)}, {"_ignored", _V(10)}}));
    objectBs.push_back(mockObject("B", 'B', 2, 0, {2, 2}, DiscreteOrientation(), {}, {{"V2", _V(5)}, {"_ignored", _V(10)}}));
    objectBs.push_back(mockObject("B", 'B', 3, 0, {2, 3}, DiscreteOrientation(), {}, {{"V3", _V(6)}, {"_ignored", _V(10)}}));
    objectCs.push_back(mockObject("C", 'C', 1, 0, {3, 1}, DiscreteOrientation(), {}, {{"V1", _V(7)}, {"_ignored", _V(10)}}));
    objectCs.push_back(mockObject("C", 'C', 2, 0, {3, 2}, DiscreteOrientation(), {}, {{"V2", _V(8)}, {"_ignored", _V(10)}}));
    objectCs.push_back(mockObject("C", 'C', 3, 0, {3, 3}, DiscreteOrientation(), {}, {{"V3", _V(9)}, {"_ignored", _V(10)}}));

    mockRTSObjects.insert(walls.begin(), walls.end());
    mockRTSObjects.insert(objectAs.begin(), objectAs.end());
    mockRTSObjects.insert(objectBs.begin(), objectBs.end());
    mockRTSObjects.insert(objectCs.begin(), objectCs.end());

    mockRTSGridData = {
        {{0, 0}, {{0, walls[0]}}},
        {{1, 0}, {{0, walls[1]}}},
        {{2, 0}, {{0, walls[2]}}},
        {{3, 0}, {{0, walls[3]}}},
        {{4, 0}, {{0, walls[4]}}},

        {{0, 1}, {{0, walls[5]}}},
        {{1, 1}, {{0, objectAs[0]}}},
        {{2, 1}, {{0, objectBs[0]}}},
        {{3, 1}, {{0, objectCs[0]}}},
        {{4, 1}, {{0, walls[6]}}},

        {{0, 2}, {{0, walls[7]}}},
        {{1, 2}, {{0, objectAs[1]}}},
        {{2, 2}, {{0, objectBs[1]}}},
        {{3, 2}, {{0, objectCs[1]}}},
        {{4, 2}, {{0, walls[8]}}},

        {{0, 3}, {{0, walls[9]}}},
        {{1, 3}, {{0, objectAs[2]}}},
        {{2, 3}, {{0, objectBs[2]}}},
        {{3, 3}, {{0, objectCs[2]}}},
        {{4, 3}, {{0, walls[10]}}},

        {{0, 4}, {{0, walls[11]}}},
        {{1, 4}, {{0, walls[12]}}},
        {{2, 4}, {{0, walls[13]}}},
        {{3, 4}, {{0, walls[14]}}},
        {{4, 4}, {{0, walls[15]}}},
    };

    mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
    EXPECT_CALL(*mockGridPtr, getWidth)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*mockGridPtr, getHeight)
        .WillRepeatedly(Return(5));

    EXPECT_CALL(*mockGridPtr, getObjects()).WillRepeatedly(ReturnRef(mockRTSObjects));
    EXPECT_CALL(*mockGridPtr, getUpdatedLocations).WillRepeatedly(ReturnRef(mockRTSUpdatedLocations));

    EXPECT_CALL(*mockGridPtr, getObjectVariableIds()).WillRepeatedly(ReturnRef(mockObjectVariableIds));
    EXPECT_CALL(*mockGridPtr, getObjectIds()).WillRepeatedly(ReturnRef(mockRTSObjectIds));

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

    EXPECT_CALL(*mockGridPtr, getPlayerCount()).WillRepeatedly(Return(3));
    EXPECT_CALL(*mockGridPtr, getGlobalVariables).WillRepeatedly(ReturnRef(globalVariables));
  }

  std::shared_ptr<MockGrid> mockGridPtr;

  const std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{{"_steps", {{0, std::make_shared<int32_t>(1)}}}};

  const std::unordered_map<std::string, uint32_t> mockRTSObjectIds = {
      {"W", 3},
      {"A", 0},
      {"B", 1},
      {"C", 2}};

  const std::unordered_map<std::string, uint32_t> mockObjectVariableIds = {
      {"V1", 0},
      {"V2", 1},
      {"V3", 2}};

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

}  // namespace griddly