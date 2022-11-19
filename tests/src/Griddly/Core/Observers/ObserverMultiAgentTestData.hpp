#include <memory>

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

#define _V(X) std::make_shared<int32_t>(X)

namespace griddly {

class ObserverMultiAgentTestData {
 public:
  ObserverMultiAgentTestData(ObserverConfig observerConfig) {
    // Make a 10x10 grid with 3 agents in it that have different orientations/grids
    mockAvatar1ObjectPtr = mockObject("avatar", 'A', 1, 0, {2, 2}, DiscreteOrientation(Direction::UP));
    mockAvatar2ObjectPtr = mockObject("avatar", 'A', 2, 0, {2, 7}, DiscreteOrientation(Direction::RIGHT));
    mockAvatar3ObjectPtr = mockObject("avatar", 'A', 3, 0, {7, 2}, DiscreteOrientation(Direction::LEFT));
    mockAvatar4ObjectPtr = mockObject("avatar", 'A', 4, 0, {7, 7}, DiscreteOrientation(Direction::DOWN));

    mockMultiAgentUpdatedLocations.insert({2, 2});
    mockMultiAgentUpdatedLocations.insert({2, 7});
    mockMultiAgentUpdatedLocations.insert({7, 2});
    mockMultiAgentUpdatedLocations.insert({7, 7});

    mockMultiAgentGridData.insert({{2, 2}, {{0, mockAvatar1ObjectPtr}}});
    mockMultiAgentGridData.insert({{2, 7}, {{0, mockAvatar2ObjectPtr}}});
    mockMultiAgentGridData.insert({{7, 2}, {{0, mockAvatar3ObjectPtr}}});
    mockMultiAgentGridData.insert({{7, 7}, {{0, mockAvatar4ObjectPtr}}});

    // 16 wall objects
    std::vector<std::shared_ptr<MockObject>> walls;

    // top
    for (int i = 0; i < 10; i++) {
      glm::ivec2 position{i, 0};
      auto wallObject = mockObject("wall", 'W', 0, -1, position);
      walls.push_back(wallObject);
      mockMultiAgentGridData.insert({position, {{0, wallObject}}});
      mockMultiAgentUpdatedLocations.insert(position);
    }

    // bottom
    for (int i = 0; i < 10; i++) {
      glm::ivec2 position{i, 9};
      auto wallObject = mockObject("wall", 'W', 0, -1, position);
      walls.push_back(wallObject);
      mockMultiAgentGridData.insert({position, {{0, wallObject}}});
      mockMultiAgentUpdatedLocations.insert(position);
    }

    // left
    for (int i = 1; i < 9; i++) {
      glm::ivec2 position{0, i};
      auto wallObject = mockObject("wall", 'W', 0, -1, position);
      walls.push_back(wallObject);
      mockMultiAgentGridData.insert({position, {{0, wallObject}}});
      mockMultiAgentUpdatedLocations.insert(position);
    }

    // right
    for (int i = 1; i < 9; i++) {
      glm::ivec2 position{9, i};
      auto wallObject = mockObject("wall", 'W', 0, -1, position);
      walls.push_back(wallObject);
      mockMultiAgentGridData.insert({position, {{0, wallObject}}});
      mockMultiAgentUpdatedLocations.insert(position);
    }

    mockMultiAgentObjects.insert(walls.begin(), walls.end());
    mockMultiAgentObjects.insert(mockAvatar1ObjectPtr);
    mockMultiAgentObjects.insert(mockAvatar2ObjectPtr);
    mockMultiAgentObjects.insert(mockAvatar3ObjectPtr);
    mockMultiAgentObjects.insert(mockAvatar4ObjectPtr);

    mockMultiAgentObjectNames = {"avatar", "wall"};

    mockGridPtr = std::make_shared<MockGrid>();
    EXPECT_CALL(*mockGridPtr, getWidth)
        .WillRepeatedly(Return(10));
    EXPECT_CALL(*mockGridPtr, getHeight)
        .WillRepeatedly(Return(10));

    EXPECT_CALL(*mockGridPtr, getObjectNames()).WillRepeatedly(Return(mockMultiAgentObjectNames));
    EXPECT_CALL(*mockGridPtr, getObjects()).WillRepeatedly(ReturnRef(mockMultiAgentObjects));
    EXPECT_CALL(*mockGridPtr, getUpdatedLocations).WillRepeatedly(ReturnRef(mockMultiAgentUpdatedLocations));
    EXPECT_CALL(*mockGridPtr, getObjectIds()).WillRepeatedly(ReturnRef(mockMultiAgentObjectIds));

    mockAgent1ObserverPtr = std::make_shared<MockObserver<>>(mockGridPtr, mockPlayerObsConfig);
    mockAgent2ObserverPtr = std::make_shared<MockObserver<>>(mockGridPtr, mockPlayerObsConfig);
    mockAgent3ObserverPtr = std::make_shared<MockObserver<>>(mockGridPtr, mockPlayerObsConfig);
    mockAgent4ObserverPtr = std::make_shared<MockObserver<>>(mockGridPtr, mockPlayerObsConfig);

    bool hasOffsets = observerConfig.gridXOffset != 0 || observerConfig.gridYOffset != 0;

    if (!observerConfig.trackAvatar && !hasOffsets) {
      EXPECT_CALL(*mockGridPtr, purgeUpdatedLocations).Times(AtLeast(1));
    }

    EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const TileObjects& {
      if (mockMultiAgentGridData.find(location) == mockMultiAgentGridData.end()) {
        return emptyObjectMap;
      } else {
        return mockMultiAgentGridData.at(location);
      }
    }));

    EXPECT_CALL(*mockGridPtr, getObject).WillRepeatedly(Invoke([this](glm::ivec2 location) -> const std::shared_ptr<Object> {
      if (mockMultiAgentGridData.find(location) == mockMultiAgentGridData.end()) {
        return nullptr;
      }

      auto objectsAt = mockMultiAgentGridData.at(location);
      if (objectsAt.size() > 0) {
        return objectsAt.at(0);
      } else {
        return nullptr;
      }
    }));

    EXPECT_CALL(*mockAvatar1ObjectPtr, getObjectRenderTileName()).WillRepeatedly(ReturnRefOfCopy("avatar" + std::to_string(0)));
    EXPECT_CALL(*mockAvatar2ObjectPtr, getObjectRenderTileName()).WillRepeatedly(ReturnRefOfCopy("avatar" + std::to_string(0)));
    EXPECT_CALL(*mockAvatar3ObjectPtr, getObjectRenderTileName()).WillRepeatedly(ReturnRefOfCopy("avatar" + std::to_string(0)));
    EXPECT_CALL(*mockAvatar4ObjectPtr, getObjectRenderTileName()).WillRepeatedly(ReturnRefOfCopy("avatar" + std::to_string(0)));

    EXPECT_CALL(*mockAgent1ObserverPtr, getObservableGrid()).WillRepeatedly(Return(PartialObservableGrid{0, 2, 1, 3}));
    EXPECT_CALL(*mockAgent2ObserverPtr, getObservableGrid()).WillRepeatedly(Return(PartialObservableGrid{1, 3, 5, 7}));
    EXPECT_CALL(*mockAgent3ObserverPtr, getObservableGrid()).WillRepeatedly(Return(PartialObservableGrid{6, 8, 2, 4}));
    EXPECT_CALL(*mockAgent4ObserverPtr, getObservableGrid()).WillRepeatedly(Return(PartialObservableGrid{7, 9, 6, 8}));

    EXPECT_CALL(*mockGridPtr, getGlobalVariables).WillRepeatedly(ReturnRef(globalVariables));
    EXPECT_CALL(*mockGridPtr, getPlayerCount()).WillRepeatedly(Return(4));
  }

  std::shared_ptr<MockGrid> mockGridPtr = nullptr;

  std::shared_ptr<MockObject> mockAvatar1ObjectPtr = nullptr;
  std::shared_ptr<MockObject> mockAvatar2ObjectPtr = nullptr;
  std::shared_ptr<MockObject> mockAvatar3ObjectPtr = nullptr;
  std::shared_ptr<MockObject> mockAvatar4ObjectPtr = nullptr;

  ObserverConfig mockPlayerObsConfig{};

  std::shared_ptr<MockObserver<>> mockAgent1ObserverPtr = nullptr;
  std::shared_ptr<MockObserver<>> mockAgent2ObserverPtr = nullptr;
  std::shared_ptr<MockObserver<>> mockAgent3ObserverPtr = nullptr;
  std::shared_ptr<MockObserver<>> mockAgent4ObserverPtr = nullptr;

  std::unordered_set<std::shared_ptr<Object>> mockMultiAgentObjects{};
  std::unordered_map<glm::ivec2, TileObjects> mockMultiAgentGridData{};
  TileObjects emptyObjectMap{};
  std::vector<std::string> mockMultiAgentObjectNames{};

  const std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{
      {"_steps", {{0, std::make_shared<int32_t>(1)}}}};

  const std::unordered_map<std::string, uint32_t> mockMultiAgentObjectIds = {
      {"avatar", 1},
      {"wall", 0}};

  std::unordered_set<glm::ivec2> mockMultiAgentUpdatedLocations;

  void verifyAndClearExpectations() {
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatar1ObjectPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatar2ObjectPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatar3ObjectPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAvatar4ObjectPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAgent1ObserverPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAgent2ObserverPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAgent3ObserverPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockAgent4ObserverPtr.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  }
};

}  // namespace griddly