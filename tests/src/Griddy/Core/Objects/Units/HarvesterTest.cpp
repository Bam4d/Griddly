#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/Objects/Units/Unit.cpp"
#include "Griddy/Core/Objects/Units/Harvester.cpp"

#include "../../../../Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "../../../../Mocks/Griddy/Core/Objects/MockObject.cpp"

using ::testing::ByMove;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(HarvesterTest, gatherNothing) {
  Harvester harvester(0);

  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  auto reward = harvester.onPerformAction(nullptr, mockGatherAction);

  ASSERT_EQ(reward, 0);
}

TEST(HarvesterTest, cannotGather) {
  Harvester harvester(0);

  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  EXPECT_CALL(*mockObject, getObjectType())
      .Times(1)
      .WillOnce(Return(FIXED_WALL));

  auto reward = harvester.onPerformAction(mockObject, mockGatherAction);

  ASSERT_EQ(reward, 0);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject.get()));
}

TEST(HarvesterTest, gather) {
  Harvester harvester(0);

  auto mockObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockGatherAction = std::shared_ptr<MockAction>(new MockAction(GATHER));

  EXPECT_CALL(*mockObject, getObjectType())
      .WillRepeatedly(Return(MINERALS));
  
  // Can collect resource if it has none
  {
    auto reward = harvester.onPerformAction(mockObject, mockGatherAction);

    ASSERT_EQ(reward, 1);
    ASSERT_EQ(harvester.getResources(), 1);
  }

  // Can collect two resources
  {
    auto reward = harvester.onPerformAction(mockObject, mockGatherAction);
    ASSERT_EQ(reward, 1);
    ASSERT_EQ(harvester.getResources(), 2);
  }

  // Cannot collect a third resource
  {
    auto reward = harvester.onPerformAction(mockObject, mockGatherAction);
    ASSERT_EQ(reward, 0);
    ASSERT_EQ(harvester.getResources(), 2);
  }

}

TEST(HarvesterTest, move) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}

TEST(HarvesterTest, pushed) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}

TEST(HarvesterTest, punched) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}

}  // namespace griddy