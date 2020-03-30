#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"
#include "Mocks/Griddy/Core/Objects/MockObject.cpp"

#include "Griddy/Core/Objects/Units/Puncher.hpp"

using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(PuncherTest, punchNothing) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  auto reward = puncher->onPerformAction(nullptr, mockPunchAction);

  ASSERT_EQ(reward, 0);
}

TEST(PuncherTest, cannotPunch) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  EXPECT_CALL(*mockObject, getObjectType())
      .WillRepeatedly(Return(FIXED_WALL));

  auto reward = puncher->onPerformAction(mockObject, mockPunchAction);

  ASSERT_EQ(reward, 0);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject.get()));
}

TEST(PuncherTest, punch) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));

  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  EXPECT_CALL(*mockObject, getObjectType())
      .WillRepeatedly(Return(PUNCHER));

  // Can collect resource if it has none
  {
    auto reward = puncher->onPerformAction(mockObject, mockPunchAction);

    ASSERT_EQ(reward, 1);
  }

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject.get()));
}

TEST(PuncherTest, moveBlockedByObject) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  puncher->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  auto reward = puncher->onPerformAction(mockObject, mockMoveAction);

  ASSERT_EQ(reward, 0);
  ASSERT_EQ(puncher->getLocation(), GridLocation(1, 1));
}

TEST(PuncherTest, moveToEmptySpace) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  puncher->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));

  EXPECT_CALL(*mockMoveAction, getDestinationLocation())
      .WillOnce(Return(GridLocation{1, 2}));

  auto reward = puncher->onPerformAction(nullptr, mockMoveAction);

  ASSERT_EQ(reward, 0);
  ASSERT_EQ(puncher->getLocation(), GridLocation(1, 2));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMoveAction.get()));
}

TEST(PuncherTest, movedByPusher) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  puncher->init({1, 1}, mockGrid);

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockPusher = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockPusher, getObjectType())
      .WillOnce(Return(PUSHER));

  EXPECT_CALL(*mockPusher, getLocation())
      .WillOnce(Return(GridLocation(0, 1)));

  auto isPerformed = puncher->onActionPerformed(mockPusher, mockMoveAction);

  ASSERT_EQ(isPerformed, true);
  ASSERT_EQ(puncher->getLocation(), GridLocation(2, 1));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPusher.get()));
}

TEST(PuncherTest, notMovedByPusher) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));

  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockPusher = std::shared_ptr<MockObject>(new MockObject());
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  puncher->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockPusher, getObjectType())
      .WillOnce(Return(PUSHER));

  EXPECT_CALL(*mockPusher, getLocation())
      .WillOnce(Return(GridLocation(0, 1)));

  EXPECT_CALL(*mockGrid, getObject(Eq(GridLocation(2, 1))))
      .WillOnce(Return(mockObject));

  auto isPerformed = puncher->onActionPerformed(mockPusher, mockMoveAction);

  ASSERT_EQ(isPerformed, false);
  ASSERT_EQ(puncher->getLocation(), GridLocation(1, 1));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPusher.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGrid.get()));
}

TEST(PuncherTest, punched) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  // This potentially could cause a bug because we are not checking that the puncher
  // is actually being punched by anything
  auto isPerformed = puncher->onActionPerformed(nullptr, mockPunchAction);

  ASSERT_EQ(isPerformed, true);
  ASSERT_EQ(puncher->getHealth(), puncher->getMaxHealth() - 1);
}

TEST(PuncherTest, destroyed) {
  auto puncher = std::shared_ptr<Puncher>(new Puncher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  puncher->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockGrid, removeObject)
      .Times(1)
      .WillOnce(Return(true));

  for (int x = puncher->getMaxHealth() - 1; x >= 0; x--) {
    auto isPerformed = puncher->onActionPerformed(nullptr, mockPunchAction);
    ASSERT_EQ(isPerformed, true);
    ASSERT_EQ(puncher->getHealth(), x);
  }

  // When there is no health left, we need to tell the grid that this object should be removed
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGrid.get()));
}

}  // namespace griddy