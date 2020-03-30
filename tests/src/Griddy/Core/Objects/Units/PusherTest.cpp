#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"
#include "Mocks/Griddy/Core/Objects/MockObject.cpp"

#include "Griddy/Core/Objects/Units/Pusher.hpp"

using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(PusherTest, moveToEmptySpace) {
  auto pusher = std::shared_ptr<Pusher>(new Pusher(0));

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  pusher->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockMoveAction, getDestinationLocation())
      .WillOnce(Return(GridLocation{1, 2}));

  auto reward = pusher->onPerformAction(nullptr, mockMoveAction);
  ASSERT_EQ(reward, 0);
  ASSERT_EQ(pusher->getLocation(), GridLocation(1, 2));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMoveAction.get()));
}

TEST(PusherTest, pushAnyObject) {
  auto pusher = std::shared_ptr<Pusher>(new Pusher(0));

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  pusher->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockMoveAction, getDestinationLocation())
      .WillOnce(Return(GridLocation{1, 2}));

  auto reward = pusher->onPerformAction(mockObject, mockMoveAction);
  ASSERT_EQ(reward, 0);
}

TEST(PusherTest, notMovedByPusher) {
  auto pusher = std::shared_ptr<Pusher>(new Pusher(0));

  auto mockMoveAction = std::shared_ptr<MockAction>(new MockAction(MOVE));
  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());

  pusher->init({1, 1}, mockGrid);

  auto isPerformed = pusher->onActionPerformed(mockObject, mockMoveAction);

  ASSERT_EQ(isPerformed, false);
  ASSERT_EQ(pusher->getLocation(), GridLocation(1, 1));
}

TEST(PusherTest, punched) {
  auto pusher = std::shared_ptr<Pusher>(new Pusher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  // This potentially could cause a bug because we are not checking that the pusher
  // is actually being punched by anything
  auto isPerformed = pusher->onActionPerformed(nullptr, mockPunchAction);

  ASSERT_EQ(isPerformed, true);
  ASSERT_EQ(pusher->getHealth(), pusher->getMaxHealth() - 1);
}

TEST(PusherTest, destroyed) {
  auto pusher = std::shared_ptr<Pusher>(new Pusher(0));
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPunchAction = std::shared_ptr<MockAction>(new MockAction(PUNCH));

  pusher->init({1, 1}, mockGrid);

  EXPECT_CALL(*mockGrid, removeObject)
      .Times(1)
      .WillOnce(Return(true));

  for (int x = pusher->getMaxHealth() - 1; x >= 0; x--) {
    auto isPerformed = pusher->onActionPerformed(nullptr, mockPunchAction);
    ASSERT_EQ(isPerformed, true);
    ASSERT_EQ(pusher->getHealth(), x);
  }

  // When there is no health left, we need to tell the grid that this object should be removed
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGrid.get()));
}

}  // namespace griddy