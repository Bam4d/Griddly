
#include <unordered_map>

#include "Griddy/Core/Grid.cpp"
#include "Mocks/Griddy/Core/GDY/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/GDY/Objects/MockObject.cpp"
// #include "Griddy/Core/GDY/Objects/Object.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(GridTest, getHeightAndWidth) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  ASSERT_EQ(grid->getWidth(), 123);
  ASSERT_EQ(grid->getHeight(), 456);
}

TEST(GridTest, initializeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockObjectPtr = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);

  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockObjectPtr = std::shared_ptr<Object>(new MockObject());
  auto mockObjectPtr2 = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);
  grid->initObject(0, {1, 2}, mockObjectPtr2);

  // The second init should be ignored because it is in the same location as the
  // first object
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockObjectPtr = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);
  grid->initObject(0, {4, 4}, mockObjectPtr);

  // There second init should be ignored because the first one is the same
  // object
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObject({4, 4}), nullptr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, removeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  auto objectLocation = GridLocation(1, 2);

  EXPECT_CALL(*mockObjectPtr, getLocation())
      .Times(1)
      .WillOnce(Return(objectLocation));

  grid->initObject(0, objectLocation, mockObjectPtr);

  ASSERT_EQ(grid->removeObject(mockObjectPtr), true);
  ASSERT_EQ(grid->getObject(objectLocation), nullptr);
  ASSERT_EQ(grid->getObjects().size(), 0);
}

TEST(GridTest, removeObjectNotInitialized) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockObjectPtr = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  ASSERT_EQ(grid->removeObject(mockObjectPtr), false);
}

TEST(GridTest, performActionOnEmptySpace) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(GridLocation(1, 0)));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(GridLocation(1, 1)));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(1, actions);

  ASSERT_EQ(reward.size(), 1);
  ASSERT_THAT(reward, ElementsAre(0));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnObjectWithNeutralPlayerId) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  uint32_t playerId = 1;
  uint32_t mockSourceObjectPlayerId = 0;

  auto mockSourceObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockSourceObjectLocation = GridLocation(1, 0);

  EXPECT_CALL(*mockSourceObjectPtr, getPlayerId())
      .Times(1)
      .WillOnce(Return(mockSourceObjectPlayerId));

  grid->initObject(mockSourceObjectPlayerId, mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(mockSourceObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(GridLocation(1, 1)));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockSourceObjectPtr, canPerformAction)
      .Times(1)
      .WillOnce(Return(false));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(0));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnObjectWithDifferentPlayerId) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  uint32_t playerId = 1;
  uint32_t mockSourceObjectPlayerId = 2;

  auto mockSourceObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockSourceObjectLocation = GridLocation(1, 0);

  EXPECT_CALL(*mockSourceObjectPtr, getPlayerId())
      .Times(1)
      .WillOnce(Return(mockSourceObjectPlayerId));

  grid->initObject(mockSourceObjectPlayerId, mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(mockSourceObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(GridLocation(1, 1)));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  // Should never need to be called
  EXPECT_CALL(*mockSourceObjectPtr, canPerformAction)
      .Times(0)
      .WillOnce(Return(false));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(0));
}

TEST(GridTest, performActionDestinationObjectNull) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  uint32_t playerId = 2;
  uint32_t mockSourceObjectPlayerId = 2;

  auto mockSourceObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockSourceObjectLocation = GridLocation(1, 0);

  EXPECT_CALL(*mockSourceObjectPtr, getPlayerId())
      .Times(1)
      .WillOnce(Return(mockSourceObjectPlayerId));

  grid->initObject(mockSourceObjectPlayerId, mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(mockSourceObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(GridLocation(1, 1)));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc(Eq(nullptr), Eq(mockActionPtr)))
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, 5}));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockSourceObjectPtr, canPerformAction)
      .Times(1)
      .WillOnce(Return(true));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(5));
}

TEST(GridTest, performActionCannotBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  uint32_t playerId = 2;

  uint32_t mockSourceObjectPlayerId = 2;
  auto mockSourceObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockSourceObjectLocation = GridLocation(0, 0);

  EXPECT_CALL(*mockSourceObjectPtr, getPlayerId())
      .Times(1)
      .WillOnce(Return(mockSourceObjectPlayerId));

  grid->initObject(mockSourceObjectPlayerId, mockSourceObjectLocation, mockSourceObjectPtr);

  uint32_t mockDestinationObjectPlayerId = 2;
  auto mockDestinationObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockDestinationObjectLocation = GridLocation(0, 1);

  grid->initObject(mockDestinationObjectPlayerId, mockDestinationObjectLocation, mockDestinationObjectPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(mockSourceObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(mockDestinationObjectLocation));

  EXPECT_CALL(*mockSourceObjectPtr, canPerformAction)
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockDestinationObjectPtr, onActionDst)
      .Times(1)
      .WillOnce(Return(BehaviourResult{true, 0}));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc)
      .Times(0);

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(0));
}

TEST(GridTest, performActionCanBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->init(123, 456);

  uint32_t playerId = 2;

  uint32_t mockSourceObjectPlayerId = 2;
  auto mockSourceObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockSourceObjectLocation = GridLocation(0, 0);

  EXPECT_CALL(*mockSourceObjectPtr, getPlayerId())
      .Times(1)
      .WillOnce(Return(mockSourceObjectPlayerId));

  grid->initObject(mockSourceObjectPlayerId, mockSourceObjectLocation, mockSourceObjectPtr);

  uint32_t mockDestinationObjectPlayerId = 2;
  auto mockDestinationObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockDestinationObjectLocation = GridLocation(0, 1);

  grid->initObject(mockDestinationObjectPlayerId, mockDestinationObjectLocation, mockDestinationObjectPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(mockSourceObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(mockDestinationObjectLocation));

  EXPECT_CALL(*mockSourceObjectPtr, canPerformAction)
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockDestinationObjectPtr, onActionDst)
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, 5}));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc(Eq(mockDestinationObjectPtr), Eq(mockActionPtr)))
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, 5}));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(10));
}

}  // namespace griddy