#include <memory>

#include "Griddly/Core/AStarPathFinder.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectRules.hpp"
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

TEST(AStarPathFinderTest, searchAllPassableSeek) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto pathFinder = std::make_shared<AStarPathFinder>(
      mockGridPtr, std::set<std::string>{}, getUpDownLeftRightActions(), PathFinderMode::SEEK);

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({1, 1}, {1, 4}, {0, 0}, 3);
  auto right = pathFinder->search({1, 1}, {4, 1}, {0, 0}, 3);
  auto down = pathFinder->search({1, 4}, {1, 1}, {0, 0}, 3);
  auto left = pathFinder->search({4, 1}, {1, 1}, {0, 0}, 3);

  ASSERT_EQ(up.actionId, 1);
  ASSERT_EQ(right.actionId, 2);
  ASSERT_EQ(down.actionId, 3);
  ASSERT_EQ(left.actionId, 4);
}

TEST(AStarPathFinderTest, searchAllPassableFlee) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto pathFinder = std::make_shared<AStarPathFinder>(
      mockGridPtr, std::set<std::string>{}, getUpDownLeftRightActions(), PathFinderMode::FLEE);

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto down = pathFinder->search({1, 1}, {1, 4}, {0, 0}, 3);
  auto left = pathFinder->search({1, 1}, {4, 1}, {0, 0}, 3);
  auto up = pathFinder->search({1, 4}, {1, 1}, {0, 0}, 3);
  auto right = pathFinder->search({4, 1}, {1, 1}, {0, 0}, 3);

  ASSERT_EQ(up.actionId, 1);
  ASSERT_EQ(right.actionId, 2);
  ASSERT_EQ(down.actionId, 3);
  ASSERT_EQ(left.actionId, 4);
}

TEST(AStarPathFinderTest, searchNoPassable) {
  auto mockObjectPtr = std::make_shared<MockObjectRules>();
  auto mockGridPtr = std::make_shared<MockGrid>();

  const std::string objectName = "impassable_object";
  EXPECT_CALL(*mockObjectPtr, getObjectName).WillRepeatedly(ReturnRef(objectName));

  auto pathFinder = std::make_shared<AStarPathFinder>(
      mockGridPtr, std::set<std::string>{objectName}, getUpDownLeftRightActions(), PathFinderMode::SEEK);

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
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto pathFinder = std::make_shared<AStarPathFinder>(
      mockGridPtr, std::set<std::string>{}, getRotateAndForwardActions(), PathFinderMode::SEEK);

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
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto pathFinder = std::make_shared<AStarPathFinder>(
      mockGridPtr, std::set<std::string>{}, getRotateAndForwardActions(), PathFinderMode::SEEK);

  TileObjects objects = {};
  EXPECT_CALL(*mockGridPtr, getObjectsAt).WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(6));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(6));

  auto up = pathFinder->search({0, 0}, {0, 5}, {1, 0}, 100);  // Agent can rotate either way here to reach the goal. So to stop flakey tests, we rotate the agent so shortest path is specific.
  auto right = pathFinder->search({0, 0}, {5, 0}, {0, 0}, 100);
  auto down = pathFinder->search({0, 5}, {0, 0}, {0, 0}, 100);
  auto left = pathFinder->search({5, 0}, {0, 0}, {0, 0}, 100);

  ASSERT_EQ(up.actionId, 3);
  ASSERT_EQ(right.actionId, 3);
  ASSERT_EQ(down.actionId, 2);
  ASSERT_EQ(left.actionId, 1);
}

// --- Regression tests for the rewritten search (detours, budgets, unreachable targets) ---------------

// A grid backed by an ASCII map: '#' cells hold an impassable "wall" object, everything else is empty.
struct AsciiGrid {
  std::vector<std::string> rows;
  std::shared_ptr<MockGrid> grid = std::make_shared<MockGrid>();
  std::shared_ptr<MockObjectRules> wall = std::make_shared<MockObjectRules>();
  std::string wallName = "wall";
  TileObjects wallTile;
  TileObjects emptyTile;

  explicit AsciiGrid(std::vector<std::string> r) : rows(std::move(r)) {
    EXPECT_CALL(*wall, getObjectName).WillRepeatedly(::testing::ReturnRef(wallName));
    wallTile = {{0, wall}};
    EXPECT_CALL(*grid, getHeight).WillRepeatedly(Return(static_cast<uint32_t>(rows.size())));
    EXPECT_CALL(*grid, getWidth).WillRepeatedly(Return(static_cast<uint32_t>(rows[0].size())));
    EXPECT_CALL(*grid, getObjectsAt).WillRepeatedly(::testing::Invoke([this](glm::ivec2 l) -> const TileObjects& {
      return rows[l.y][l.x] == '#' ? wallTile : emptyTile;
    }));
  }
};

// Follow the path finder step by step (applying each returned action) and count moves to the goal.
static int32_t walkToGoal(AStarPathFinder& pf, const ActionInputsDefinition& inputs, glm::ivec2 start, glm::ivec2 goal, glm::ivec2 facing, uint32_t maxDepth, int32_t maxSteps = 500) {
  auto location = start;
  auto orientation = facing;
  for (int32_t step = 0; step < maxSteps; step++) {
    if (location == goal) return step;
    auto out = pf.search(location, goal, orientation, maxDepth);
    if (out.actionId == 0) return -1;
    const auto& mapping = inputs.inputMappings.at(out.actionId);
    auto rot = DiscreteOrientation(orientation).getRotationMatrix();
    location += inputs.relative ? mapping.vectorToDest * rot : mapping.vectorToDest;
    orientation = inputs.relative ? mapping.orientationVector * rot : mapping.orientationVector;
  }
  return -2;
}

TEST(AStarPathFinderTest, searchFindsDetourAroundWalls) {
  // The straight line to the goal is blocked; the shortest path leaves through the gap (5 moves).
  AsciiGrid g({"........",
               "..####..",
               "..#S.#..",
               "..#..#..",
               "..##.#..",
               "...G....",
               "........"});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  // S=(3,2) G=(3,5): right, down through the gap at (4,4), down, left = 5 (BFS optimum)
  ASSERT_EQ(walkToGoal(pf, inputs, {3, 2}, {3, 5}, {0, 0}, 10000), 5);
}

TEST(AStarPathFinderTest, searchLongMazeBeyondOldDepthBudget) {
  // A serpentine corridor: the goal is 53 moves away (BFS optimum), well past the old 100-node budget.
  AsciiGrid g({"S.........",
               "#########.",
               "..........",
               ".#########",
               "..........",
               "#########.",
               "..........",
               ".#########",
               ".........G"});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  ASSERT_EQ(walkToGoal(pf, inputs, {0, 0}, {9, 8}, {0, 0}, 10000), 53);
}

TEST(AStarPathFinderTest, searchRotationActionsThroughMaze) {
  AsciiGrid g({"S.........",
               "#########.",
               "..........",
               ".#########",
               ".........G"});
  auto inputs = getRotateAndForwardActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  // facing right: 9 fwd, turn right, 2 fwd, turn right, 9 fwd, turn left, 2 fwd, turn left, 9 fwd = 35
  ASSERT_EQ(walkToGoal(pf, inputs, {0, 0}, {9, 4}, {1, 0}, 10000), 35);
}

TEST(AStarPathFinderTest, searchUnreachableGoalMovesCloserButNeverIntoWalls) {
  AsciiGrid g({"S...#...",
               "....#.G.",
               "....#..."});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  // Unreachable: it heads to the closest reachable cell (next to the wall) and then stays (action 0).
  glm::ivec2 location{0, 0};
  for (int i = 0; i < 20; i++) {
    auto out = pf.search(location, {6, 1}, {0, 0}, 10000);
    if (out.actionId == 0) break;
    location += inputs.inputMappings.at(out.actionId).vectorToDest;
    ASSERT_NE(g.rows[location.y][location.x], '#');
  }
  ASSERT_EQ(location.x, 3);
}

TEST(AStarPathFinderTest, searchTargetCellIsEnterableEvenIfImpassable) {
  // The chased object may itself be in the impassable set; the search must still path next to/into it.
  AsciiGrid g({"S..#"});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  ASSERT_EQ(pf.search({0, 0}, {3, 0}, {0, 0}, 10000).actionId, 2);
  ASSERT_EQ(walkToGoal(pf, inputs, {0, 0}, {3, 0}, {0, 0}, 10000), 3);
}

TEST(AStarPathFinderTest, searchAtGoalReturnsNoAction) {
  AsciiGrid g({"...."});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::SEEK);
  ASSERT_EQ(pf.search({1, 0}, {1, 0}, {0, 0}, 10000).actionId, 0);
}

TEST(AStarPathFinderTest, searchFleeRunsAwayAroundWalls) {
  AsciiGrid g({"......",
               ".####.",
               ".#..#.",
               ".#..#.",
               "......"});
  auto inputs = getUpDownLeftRightActions();
  AStarPathFinder pf(g.grid, {"wall"}, inputs, PathFinderMode::FLEE);
  // Chaser at (0,0); the fleeing object at (1,0) should move away (right), not into the wall below.
  auto out = pf.search({1, 0}, {0, 0}, {0, 0}, 10000);
  ASSERT_EQ(out.actionId, 2);
}

}  // namespace griddly