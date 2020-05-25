
#include <unordered_map>

#include "Griddly/Core/Grid.cpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

TEST(GridTest, getHeightAndWidth) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  ASSERT_EQ(grid->getWidth(), 123);
  ASSERT_EQ(grid->getHeight(), 456);
}

TEST(GridTest, initializeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);

  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockObjectPtr2 = std::shared_ptr<MockObject>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);
  grid->initObject(0, {1, 2}, mockObjectPtr2);

  // The second init should be ignored because it is in the same location as the
  // first object
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwiceDifferentZ) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());
  auto mockObjectPtr2 = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockObjectPtr, getZIdx())
      .Times(1)
      .WillOnce(Return(1));

  EXPECT_CALL(*mockObjectPtr2, getZIdx())
      .Times(1)
      .WillOnce(Return(2));

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject(0, {1, 2}, mockObjectPtr);
  grid->initObject(0, {1, 2}, mockObjectPtr2);

  // Because the objects have different zindexes they can exist in the same grid position.
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr2);
  ASSERT_EQ(grid->getObjects().size(), 2);
  ASSERT_EQ(grid->getObjectsAt({1, 2}).size(), 2);
}

TEST(GridTest, initializeObjectTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

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
  grid->resetMap(123, 456);

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  auto objectLocation = GridLocation(1, 2);

  EXPECT_CALL(*mockObjectPtr, getLocation())
      .Times(1)
      .WillOnce(Return(objectLocation));

  grid->initObject(0, objectLocation, mockObjectPtr);

  ASSERT_EQ(grid->removeObject(mockObjectPtr), true);
  ASSERT_EQ(grid->getObject(objectLocation), nullptr);
  ASSERT_TRUE(grid->getUpdatedLocations().size() > 0);
  ASSERT_EQ(grid->getObjects().size(), 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectPtr.get()));
}

TEST(GridTest, removeObjectNotInitialized) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto objectLocation = GridLocation{4, 4};

  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockObjectPtr, getLocation())
      .Times(1)
      .WillOnce(Return(objectLocation));

  ASSERT_EQ(grid->getObjects().size(), 0);
  ASSERT_EQ(grid->getUpdatedLocations().size(), 0);
  ASSERT_EQ(grid->removeObject(mockObjectPtr), false);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectPtr.get()));
}

TEST(GridTest, performActionOnEmptySpace) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

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
  grid->resetMap(123, 456);

  uint32_t playerId = 0;
  uint32_t mockSourceObjectPlayerId = 1;

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

  EXPECT_CALL(*mockSourceObjectPtr, checkPreconditions)
      .Times(1)
      .WillOnce(Return(false));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(0));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnObjectWithDifferentPlayerId) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

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
  EXPECT_CALL(*mockSourceObjectPtr, checkPreconditions)
      .Times(0)
      .WillOnce(Return(false));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(0));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionDestinationObjectNull) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

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

  EXPECT_CALL(*mockSourceObjectPtr, checkPreconditions)
      .Times(1)
      .WillOnce(Return(true));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ElementsAre(5));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionCannotBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

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

  EXPECT_CALL(*mockSourceObjectPtr, checkPreconditions)
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

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionCanBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

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

  EXPECT_CALL(*mockSourceObjectPtr, checkPreconditions)
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

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, objectCounters) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::shared_ptr<Object>>> objects;

  std::string objectName = "cat";
  for (uint32_t p = 0; p < 10; p++) {
    for (uint32_t o = 0; o < 5; o++) {
      auto mockObject = std::shared_ptr<MockObject>(new MockObject());

      GridLocation location = {p, o};
      EXPECT_CALL(*mockObject, init).Times(1);
      EXPECT_CALL(*mockObject, getZIdx).WillRepeatedly(Return(0));
      EXPECT_CALL(*mockObject, getLocation).WillRepeatedly(Return(location));

      EXPECT_CALL(*mockObject, getPlayerId())
          .WillRepeatedly(Return(p));

      EXPECT_CALL(*mockObject, getObjectName())
          .WillRepeatedly(Return(objectName));

      grid->initObject(p, location, mockObject);

      objects[p][o] = mockObject;
    }
  }

  auto objectCounter = grid->getObjectCounter(objectName);
  for (int p = 0; p < 10; p++) {
    ASSERT_EQ(*objectCounter[p], 5);
  }

  int playerToRemoveObjectsFrom = 5;
  for (int r = 0; r < 5; r++) {
    grid->removeObject(objects[playerToRemoveObjectsFrom][r]);
  }

  auto objectCounter1 = grid->getObjectCounter(objectName);
  for (int p = 0; p < 10; p++) {
    if (p == playerToRemoveObjectsFrom) {
      ASSERT_EQ(*objectCounter1[p], 0);
    } else {
      ASSERT_EQ(*objectCounter1[p], 5);
    }
  }
}

TEST(GridTest, objectCountersEmpty) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto objectCounter = grid->getObjectCounter("object");

  ASSERT_EQ(*objectCounter[0], 0);
}

}  // namespace griddly