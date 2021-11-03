#include <memory>

#include "Griddly/Core/AStarPathFinder.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

ActionInputsDefinition getUpDownLeftRightActions() {
  ActionInputsDefinition definition;
  definition.inputMappings = {
      {1, {{0, 1}}}, {2, {{1, 0}}}, {3, {{0, -1}}}, {4, {{-1, 0}}}};
  definition.relative = false;
  definition.internal = false;
  definition.mapToGrid = false;

  return definition;
}

ActionInputsDefinition getRotateAndForwardActions() {
  // InputMapping:
  //     Inputs:
  //       1:
  //         Description: Rotate left
  //         OrientationVector: [-1, 0]
  //       2:
  //         Description: Move forwards
  //         OrientationVector: [0, -1]
  //         VectorToDest: [0, -1]
  //       3:
  //         Description: Rotate right
  //         OrientationVector: [1, 0]
  //     Relative: true

  ActionInputsDefinition definition;
  definition.inputMappings = {// Rotate left
                              {1, {{0, 0}, {-1, 0}}},

                              // Go forward
                              {2, {{0, -1}, {0, -1}}},

                              // Rotate right
                              {3, {{0, 0}, {1, 0}}}};
  definition.relative = true;
  definition.internal = false;
  definition.mapToGrid = false;

  return definition;
}

TEST(AStarPathFinderTest, searchAllPassable) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto pathFinder = std::shared_ptr<AStarPathFinder>(
      new AStarPathFinder(mockGridPtr, {}, getUpDownLeftRightActions()));

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({0, 0}, {0, 5}, {0, 0}, 100);
  auto right = pathFinder->search({0, 0}, {5, 0}, {0, 0}, 100);
  auto down = pathFinder->search({0, 5}, {0, 0}, {0, 0}, 100);
  auto left = pathFinder->search({5, 0}, {0, 0}, {0, 0}, 100);

  ASSERT_EQ(up.actionId, 1);
  ASSERT_EQ(right.actionId, 2);
  ASSERT_EQ(down.actionId, 3);
  ASSERT_EQ(left.actionId, 4);
}

TEST(AStarPathFinderTest, searchNoPassable) {
  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  std::string objectName = "impassable_object";
  EXPECT_CALL(*mockObjectPtr, getObjectName).WillRepeatedly(Return(objectName));

  auto pathFinder = std::shared_ptr<AStarPathFinder>(new AStarPathFinder(
      mockGridPtr, {objectName}, getUpDownLeftRightActions()));

  TileObjects objects = {{0, mockObjectPtr}};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({0, 0}, {0, 5}, {0, 0}, 100);
  auto right = pathFinder->search({0, 0}, {5, 0}, {0, 0}, 100);
  auto down = pathFinder->search({0, 5}, {0, 0}, {0, 0}, 100);
  auto left = pathFinder->search({5, 0}, {0, 0}, {0, 0}, 100);

  ASSERT_EQ(up.actionId, 0);
  ASSERT_EQ(right.actionId, 0);
  ASSERT_EQ(down.actionId, 0);
  ASSERT_EQ(left.actionId, 0);
}

TEST(AStarPathFinderTest, searchRotationActionsFacingGoal) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto pathFinder = std::shared_ptr<AStarPathFinder>(
      new AStarPathFinder(mockGridPtr, {}, getRotateAndForwardActions()));

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({0, 0}, {0, 5}, {0, 1}, 100);
  auto right = pathFinder->search({0, 0}, {5, 0}, {1, 0}, 100);
  auto down = pathFinder->search({0, 5}, {0, 0}, {0, -1}, 100);
  auto left = pathFinder->search({5, 0}, {0, 0}, {-1, 0}, 100);

  ASSERT_EQ(up.actionId, 2);
  ASSERT_EQ(right.actionId, 2);
  ASSERT_EQ(down.actionId, 2);
  ASSERT_EQ(left.actionId, 2);
}

TEST(AStarPathFinderTest, searchRotationActions) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto pathFinder = std::shared_ptr<AStarPathFinder>(
      new AStarPathFinder(mockGridPtr, {}, getRotateAndForwardActions()));

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({0, 0}, {0, 5}, {1, 0}, 100); // Agent can rotate either way here to reach the goal. So to stop flakey tests, we rotate the agent so shortest path is specific.
  auto right = pathFinder->search({0, 0}, {5, 0}, {0, 0}, 100);
  auto down = pathFinder->search({0, 5}, {0, 0}, {0, 0}, 100);
  auto left = pathFinder->search({5, 0}, {0, 0}, {0, 0}, 100);

  ASSERT_EQ(up.actionId, 3);
  ASSERT_EQ(right.actionId, 3);
  ASSERT_EQ(down.actionId, 2);
  ASSERT_EQ(left.actionId, 1);
}

} // namespace griddly