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

class ObserverTestData {
 public:
  ObserverTestData(ObserverConfig observerConfig, DiscreteOrientation orientation, bool trackAvatar) {
    mockAvatarObjectPtr = mockObject("avatar", 'A', 1, 0, {2, 2});

    // 16 wall objects 
    std::vector<std::shared_ptr<MockObject>> walls;
    walls.push_back(mockObject("mo1", 'W', 1, 0, {0, 0}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {1, 0}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {2, 0}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {3, 0}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {4, 0}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {0, 1}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {4, 1}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {0, 2}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {4, 2}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {0, 3}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {4, 3}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {0, 4}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {1, 4}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {2, 4}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {3, 4}));
    walls.push_back(mockObject("mo1", 'W', 1, 0, {4, 4}));

    std::vector<std::shared_ptr<MockObject>> tiles;
    tiles.push_back(mockObject("mo2", 'P', 1, 0, {1, 1}));
    tiles.push_back(mockObject("mo2", 'P', 1, 0, {1, 2}));
    tiles.push_back(mockObject("mo2", 'P', 1, 0, {3, 3}));

    std::vector<std::shared_ptr<MockObject>> bears;
    bears.push_back(mockObject("mo3", 'Q', 1, 0, {3, 1}));
    bears.push_back(mockObject("mo3", 'Q', 1, 0, {3, 2}));
    bears.push_back(mockObject("mo3", 'Q', 1, 0, {1, 3}));
    
    mockSinglePlayerObjects.insert(walls.begin(), walls.end());
    mockSinglePlayerObjects.insert(tiles.begin(), tiles.end());
    mockSinglePlayerObjects.insert(bears.begin(), bears.end());
    mockSinglePlayerObjects.insert(mockAvatarObjectPtr);

    mockSinglePlayerGridData = {
        {{0, 0}, {{0, walls[0]}}},
        {{1, 0}, {{0, walls[1]}}},
        {{2, 0}, {{0, walls[2]}}},
        {{3, 0}, {{0, walls[3]}}},
        {{4, 0}, {{0, walls[4]}}},

        {{0, 1}, {{0, walls[5]}}},
        {{1, 1}, {{0, tiles[0]}}},
        {{2, 1}, {}},
        {{3, 1}, {{0, bears[0]}}},
        {{4, 1}, {{0, walls[6]}}},

        {{0, 2}, {{0, walls[7]}}},
        {{1, 2}, {{0, tiles[1]}}},
        {{2, 2}, {{0, mockAvatarObjectPtr}}},
        {{3, 2}, {{0, bears[1]}}},
        {{4, 2}, {{0, walls[8]}}},

        {{0, 3}, {{0, walls[9]}}},
        {{1, 3}, {{0, bears[2]}}},
        {{2, 3}, {}},
        {{3, 3}, {{0, tiles[2]}}},
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
      if (mockSinglePlayerGridData.find(location) == mockSinglePlayerGridData.end()) {
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

    EXPECT_CALL(*mockGridPtr, getGlobalVariables).WillRepeatedly(ReturnRef(globalVariables));
     EXPECT_CALL(*mockGridPtr, getPlayerCount()).WillRepeatedly(Return(1));
  }

  std::shared_ptr<MockGrid> mockGridPtr;

  std::shared_ptr<MockObject> mockAvatarObjectPtr;

  std::unordered_set<std::shared_ptr<Object>> mockSinglePlayerObjects;
  std::unordered_map<glm::ivec2, TileObjects> mockSinglePlayerGridData;

  const std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{{"_steps", {{0, std::make_shared<int32_t>(1)}}}};

  const std::unordered_map<std::string, uint32_t> mockSinglePlayerObjectIds = {
      {"avatar", 3},
      {"mo1", 0},
      {"mo2", 1},
      {"mo3", 2}};

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