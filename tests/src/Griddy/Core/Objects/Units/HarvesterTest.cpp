#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"
#include "Mocks/Griddy/Core/Objects/MockObject.cpp"

#include "Griddy/Core/Objects/Units/Harvester.hpp"

using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(HarvesterTest, gatherNothing) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  auto reward = harvester->onPerformAction(nullptr, mockGatherAction);

  ASSERT_EQ(reward, 0);
}

TEST(HarvesterTest, cannotGather) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  EXPECT_CALL(*mockObject, getObjectType())
      .Times(1)
      .WillOnce(Return(FIXED_WALL));

  auto reward = harvester->onPerformAction(mockObject, mockGatherAction);

  ASSERT_EQ(reward, 0);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject.get()));
}

TEST(HarvesterTest, gather) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));

  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  EXPECT_CALL(*mockObject, getObjectType())
      .WillRepeatedly(Return(MINERALS));

  // Can collect resource if it has none
  {
    auto reward = harvester->onPerformAction(mockObject, mockGatherAction);

    ASSERT_EQ(reward, 1);
    ASSERT_EQ(harvester->getResources(), 1);
  }

  // Can collect two resources
  {
    auto reward = harvester->onPerformAction(mockObject, mockGatherAction);
    ASSERT_EQ(reward, 1);
    ASSERT_EQ(harvester->getResources(), 2);
  }

  // Cannot collect a third resource
  {
    auto reward = harvester->onPerformAction(mockObject, mockGatherAction);
    ASSERT_EQ(reward, 0);
    ASSERT_EQ(harvester->getResources(), 2);
  }

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject.get()));
}

TEST(HarvesterTest, moveBlockedByObject) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  harvester->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  auto reward = harvester->onPerformAction(mockObject, mockMoveAction);

  ASSERT_EQ(reward, 0);
  ASSERT_EQ(harvester->getLocation(), GridLocation(1, 1));
}

TEST(HarvesterTest, moveToEmptySpace) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  harvester->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));

  EXPECT_CALL(*mockMoveAction, getDestinationLocation())
      .WillOnce(Return(GridLocation{1, 2}));

  auto reward = harvester->onPerformAction(nullptr, mockMoveAction);

  ASSERT_EQ(reward, 0);
  ASSERT_EQ(harvester->getLocation(), GridLocation(1, 2));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMoveAction.get()));
}

TEST(HarvesterTest, movedByPusher) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  harvester->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockPusher = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockPusher, getObjectType())
      .WillOnce(Return(PUSHER));

  EXPECT_CALL(*mockPusher, getLocation())
      .WillOnce(Return(GridLocation(0, 1)));

  auto isPerformed = harvester->onActionPerformed(mockPusher, mockMoveAction);

  ASSERT_EQ(isPerformed, true);
  ASSERT_EQ(harvester->getLocation(), GridLocation(2, 1));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPusher.get()));
}

TEST(HarvesterTest, notMovedByPusher) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));

  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockPusher = std::shared_ptr<MockObject>(new MockObject());
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  harvester->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockPusher, getObjectType())
      .WillOnce(Return(PUSHER));

  EXPECT_CALL(*mockPusher, getLocation())
      .WillOnce(Return(GridLocation(0, 1)));

  EXPECT_CALL(*mockGrid, getObject(Eq(GridLocation(2, 1))))
      .WillOnce(Return(mockObject));

  auto isPerformed = harvester->onActionPerformed(mockPusher, mockMoveAction);

  ASSERT_EQ(isPerformed, false);
  ASSERT_EQ(harvester->getLocation(), GridLocation(1, 1));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPusher.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGrid.get()));
}

TEST(HarvesterTest, punched) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  // This potentially could cause a bug because we are not checking that the harvester
  // is actually being punched by anything
  auto isPerformed = harvester->onActionPerformed(nullptr, mockPunchAction);

  ASSERT_EQ(isPerformed, true);
  ASSERT_EQ(harvester->getHealth(), harvester->getMaxHealth() - 1);
}

TEST(HarvesterTest, destroyed) {
  auto harvester = std::shared_ptr<Harvester>(new Harvester(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  harvester->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockGrid, removeObject)
      .Times(1)
      .WillOnce(Return(true));

  for (int x = harvester->getMaxHealth() - 1; x >= 0; x--) {
    auto isPerformed = harvester->onActionPerformed(nullptr, mockPunchAction);
    ASSERT_EQ(isPerformed, true);
    ASSERT_EQ(harvester->getHealth(), x);
  }

  // When there is no health left, we need to tell the grid that this object should be removed
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGrid.get()));
}

}  // namespace griddy