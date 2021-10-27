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

TEST(AStarPathFinderTest, searchAllPassable) {
  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto pathFinder = std::shared_ptr<AStarPathFinder>(new AStarPathFinder(mockGridPtr, {}, getUpDownLeftRightActions()));

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  // auto up = pathFinder->search({0, 0}, {0, 5}, 100);
  auto right = pathFinder->search({0, 0}, {5, 0}, 100);
  // auto down = pathFinder->search({0, 5}, {0, 0}, 100);
  // auto left = pathFinder->search({5, 0}, {0, 0}, 100);

  // ASSERT_EQ(up.actionId, 1);
  ASSERT_EQ(right.actionId, 2);
  // ASSERT_EQ(down.actionId, 3);
  // ASSERT_EQ(left.actionId, 4);
}

TEST(AStarPathFinderTest, searchNoPassable) {}

TEST(AStarPathFinderTest, searchSelfPassable) {}

}  // namespace griddly