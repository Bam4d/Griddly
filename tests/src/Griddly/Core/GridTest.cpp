
#include <unordered_map>

#include "Griddly/Core/Grid.cpp"
#include "Griddly/Core/TestUtils/common.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ContainerEq;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

MATCHER_P(ActionEventMatcher, expectedEvent, "") {
  auto actualEvent = arg;

  return expectedEvent.actionName == arg.actionName &&
         expectedEvent.sourceObjectName == arg.sourceObjectName &&
         expectedEvent.destObjectName == arg.destObjectName &&
         expectedEvent.sourceLocation == arg.sourceLocation &&
         expectedEvent.destLocation == arg.destLocation &&
         expectedEvent.tick == arg.tick &&
         expectedEvent.rewards.size() == arg.rewards.size();
}

TEST(GridTest, getHeightAndWidth) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  ASSERT_EQ(grid->getWidth(), 123);
  ASSERT_EQ(grid->getHeight(), 456);
}

TEST(GridTest, initializeAvatarObjectDefaultPlayer) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("player_1_avatar");

  EXPECT_CALL(*mockObjectPtr, isPlayerAvatar())
      .WillOnce(Return(true));

  grid->initObject("player_1_avatar", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);

  auto avatarObjects = grid->getPlayerAvatarObjects();

  ASSERT_EQ(avatarObjects[1], mockObjectPtr);
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeAvatarObjectSpecificPlayer) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("player_1_avatar", 3);

  EXPECT_CALL(*mockObjectPtr, isPlayerAvatar())
      .WillOnce(Return(true));

  grid->initObject("player_1_avatar", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);

  auto avatarObjects = grid->getPlayerAvatarObjects();

  ASSERT_EQ(avatarObjects[3], mockObjectPtr);
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("object_1");
  grid->initObject("object_1", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);

  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("object_1");
  auto mockObjectPtr2 = mockObject("object_2");

  grid->initObject("object_1", {});
  grid->initObject("object_2", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);
  grid->addObject({1, 2}, mockObjectPtr2);

  // The second init should be ignored because it is in the same location as the
  // first object
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwiceDifferentZ) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("object1", 1, 0);
  auto mockObjectPtr2 = mockObject("object2", 1, 1);

  grid->initObject("object1", {});
  grid->initObject("object2", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);
  grid->addObject({1, 2}, mockObjectPtr2);

  // Because the objects have different zindexes they can exist in the same grid position.
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr2);
  ASSERT_EQ(grid->getObjects().size(), 2);
  ASSERT_EQ(grid->getObjectsAt({1, 2}).size(), 2);
}

TEST(GridTest, initializeObjectTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto mockObjectPtr = mockObject("object");
  grid->initObject("object", {});

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->addObject({1, 2}, mockObjectPtr);
  grid->addObject({4, 4}, mockObjectPtr);

  // There second init should be ignored because the first one is the same
  // object
  ASSERT_EQ(grid->getObject({1, 2}), mockObjectPtr);
  ASSERT_EQ(grid->getObject({4, 4}), nullptr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, removeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto playerId = 1;

  auto objectLocation = glm::ivec2(1, 2);
  auto mockObjectPtr = mockObject("object", playerId, 0, objectLocation);
  grid->initObject("object", {});

  grid->addObject(objectLocation, mockObjectPtr);

  ASSERT_EQ(grid->removeObject(mockObjectPtr), true);
  ASSERT_EQ(grid->getObject(objectLocation), nullptr);
  ASSERT_TRUE(grid->getUpdatedLocations(1).size() > 0);
  ASSERT_EQ(grid->getObjects().size(), 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectPtr.get()));
}

TEST(GridTest, removeObjectNotInitialized) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  auto objectLocation = glm::ivec2{4, 4};

  auto mockObjectPtr = mockObject("object", 1, 0, objectLocation);
  grid->initObject("object", {});

  ASSERT_EQ(grid->getObjects().size(), 0);
  ASSERT_EQ(grid->getUpdatedLocations(1).size(), 0);
  ASSERT_EQ(grid->removeObject(mockObjectPtr), false);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectPtr.get()));
}

TEST(GridTest, performActionDefaultObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  auto mockActionDestinationLocation = glm::ivec2(2, 0);
  auto mockActionSourceLocation = glm::ivec2(2, 0);

  auto mockActionPtr = mockAction("action", mockActionSourceLocation, mockActionDestinationLocation);

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(1, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 1;
  gridEvent.sourceObjectPlayerId = 0;
  gridEvent.destinationObjectPlayerId = 0;
  gridEvent.sourceObjectName = "_empty";
  gridEvent.destObjectName = "_empty";
  gridEvent.sourceLocation = mockActionSourceLocation; 
  gridEvent.destLocation = mockActionDestinationLocation;
  gridEvent.rewards = {};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnEmptySpace) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  auto mockActionSourceLocation = glm::ivec2(1, 0);
  auto mockActionDestinationLocation = glm::ivec2(2, 0);

  auto mockActionPtr = mockAction("action", mockActionSourceLocation, mockActionDestinationLocation);

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(1, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 1;
  gridEvent.sourceObjectPlayerId = 0;
  gridEvent.destinationObjectPlayerId = 0;
  gridEvent.sourceObjectName = "_empty";
  gridEvent.destObjectName = "_empty";
  gridEvent.sourceLocation = mockActionSourceLocation;
  gridEvent.destLocation = mockActionDestinationLocation;
  gridEvent.rewards = {};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnObjectWithNeutralPlayerId) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  uint32_t playerId = 0;
  uint32_t mockSourceObjectPlayerId = 1;
  auto mockSourceObjectLocation = glm::ivec2(1, 0);
  auto actionDestinationLocation = glm::ivec2(2, 0);

  auto mockSourceObjectPtr = mockObject("srcObject", mockSourceObjectPlayerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, actionDestinationLocation);

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockSourceObjectPtr, isValidAction)
      .Times(1)
      .WillOnce(Return(false));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 0;
  gridEvent.sourceObjectPlayerId = mockSourceObjectPlayerId;
  gridEvent.destinationObjectPlayerId = 0;
  gridEvent.sourceObjectName = "srcObject";
  gridEvent.destObjectName = "_empty";
  gridEvent.sourceLocation = mockSourceObjectLocation;
  gridEvent.destLocation = actionDestinationLocation;
  gridEvent.rewards = {};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionOnObjectWithDifferentPlayerId) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  uint32_t playerId = 1;
  uint32_t mockSourceObjectPlayerId = 2;

  auto mockSourceObjectLocation = glm::ivec2(1, 0);
  auto mockSourceObjectPtr = mockObject("srcObject", mockSourceObjectPlayerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, glm::ivec2{2, 0});

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  // Should never need to be called
  EXPECT_CALL(*mockSourceObjectPtr, isValidAction).Times(0);

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{}));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionDestinationObjectNull) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  uint32_t playerId = 2;
  uint32_t mockSourceObjectPlayerId = 2;
  auto mockSourceObjectLocation = glm::ivec2(1, 0);
  auto actionDestinationLocation = glm::ivec2(2, 0);

  auto mockSourceObjectPtr = mockObject("srcObject", mockSourceObjectPlayerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, actionDestinationLocation);

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc(Eq("_empty"), Eq(mockActionPtr)))
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, {{3, 5}}}));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockSourceObjectPtr, isValidAction)
      .Times(1)
      .WillOnce(Return(true));

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{{3,5}}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 2;
  gridEvent.sourceObjectPlayerId = mockSourceObjectPlayerId;
  gridEvent.destinationObjectPlayerId = 0;
  gridEvent.sourceObjectName = "srcObject";
  gridEvent.destObjectName = "_empty";
  gridEvent.sourceLocation = mockSourceObjectLocation;
  gridEvent.destLocation = actionDestinationLocation;
  gridEvent.rewards = {{3,5}};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionCannotBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  uint32_t playerId = 2;

  uint32_t mockSourceObjectPlayerId = 2;
  auto mockSourceObjectLocation = glm::ivec2(0, 0);
  auto mockSourceObjectPtr = mockObject("srcObject", mockSourceObjectPlayerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  uint32_t mockDestinationObjectPlayerId = 2;
  auto mockDestinationObjectLocation = glm::ivec2(0, 1);
  auto mockDestinationObjectPtr = mockObject("dstObject", mockDestinationObjectPlayerId, 0, mockDestinationObjectLocation);
  grid->initObject("dstObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);
  grid->addObject(mockDestinationObjectLocation, mockDestinationObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, mockDestinationObjectPtr);

  EXPECT_CALL(*mockSourceObjectPtr, isValidAction)
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockDestinationObjectPtr, onActionDst)
      .Times(1)
      .WillOnce(Return(BehaviourResult{true}));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc)
      .Times(0);

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 2;
  gridEvent.sourceObjectPlayerId = mockSourceObjectPlayerId;
  gridEvent.destinationObjectPlayerId = mockDestinationObjectPlayerId;
  gridEvent.sourceObjectName = "srcObject";
  gridEvent.destObjectName = "dstObject";
  gridEvent.sourceLocation = mockSourceObjectLocation;
  gridEvent.destLocation = mockDestinationObjectLocation;
  gridEvent.rewards = {};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionCanBePerformedOnDestinationObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  uint32_t playerId = 2;

  uint32_t mockSourceObjectPlayerId = 2;
  auto mockSourceObjectLocation = glm::ivec2(0, 0);
  auto mockSourceObjectPtr = mockObject("srcObject", mockSourceObjectPlayerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  uint32_t mockDestinationObjectPlayerId = 4;
  auto mockDestinationObjectLocation = glm::ivec2(0, 1);
  auto mockDestinationObjectPtr = mockObject("dstObject", mockDestinationObjectPlayerId, 0, mockDestinationObjectLocation);
  grid->initObject("dstObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);
  grid->addObject(mockDestinationObjectLocation, mockDestinationObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, mockDestinationObjectPtr);

  EXPECT_CALL(*mockSourceObjectPtr, isValidAction)
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockDestinationObjectPtr, onActionDst)
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, {{2, 5}}}));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc(Eq("dstObject"), Eq(mockActionPtr)))
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, {{4, 5}}}));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(playerId, actions);

  ASSERT_THAT(reward, ContainerEq(std::unordered_map<uint32_t, int32_t>{{2, 5}, {4, 5}}));

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 2;
  gridEvent.sourceObjectPlayerId = mockSourceObjectPlayerId;
  gridEvent.destinationObjectPlayerId = mockDestinationObjectPlayerId;
  gridEvent.sourceObjectName = "srcObject";
  gridEvent.destObjectName = "dstObject";
  gridEvent.sourceLocation = mockSourceObjectLocation;
  gridEvent.destLocation = mockDestinationObjectLocation;
  gridEvent.rewards = {{2, 5}, {4, 5}};
  gridEvent.tick = 0;
  gridEvent.delay = 0;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, performActionDelayed) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);
  grid->enableHistory(true);

  uint32_t playerId = 2;

  auto mockSourceObjectLocation = glm::ivec2(0, 0);
  auto mockSourceObjectPtr = mockObject("srcObject", playerId, 0, mockSourceObjectLocation);
  grid->initObject("srcObject", {});

  auto mockDestinationObjectLocation = glm::ivec2(0, 1);
  auto mockDestinationObjectPtr = mockObject("dstObject", playerId, 0, mockDestinationObjectLocation);
  grid->initObject("dstObject", {});

  grid->addObject(mockSourceObjectLocation, mockSourceObjectPtr);
  grid->addObject(mockDestinationObjectLocation, mockDestinationObjectPtr);

  auto mockActionPtr = mockAction("action", mockSourceObjectPtr, mockDestinationObjectPtr);

  // Delay the action for 10
  EXPECT_CALL(*mockActionPtr, getDelay())
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockSourceObjectPtr, isValidAction)
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockDestinationObjectPtr, onActionDst)
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, {{playerId, 5}}}));

  EXPECT_CALL(*mockSourceObjectPtr, onActionSrc(Eq("dstObject"), Eq(mockActionPtr)))
      .Times(1)
      .WillOnce(Return(BehaviourResult{false, {{playerId, 6}}}));

  auto actions = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  auto reward = grid->performActions(playerId, actions);

  ASSERT_EQ(reward.size(), 0);

  ASSERT_EQ(grid->getHistory().size(), 0);

  // For the next 10 ticks, there are no rewards
  for (int i = 0; i < 9; i++) {
    auto delayedRewards = grid->update();
    ASSERT_EQ(delayedRewards.size(), 0);
  }

  // on the 10th tick we recieve the reward as the action is executed
  auto delayedRewards = grid->update();
  ASSERT_EQ(delayedRewards.size(), 1);

  std::unordered_map<uint32_t, int32_t> expectedRewards{{playerId, 11}};
  ASSERT_EQ(delayedRewards, expectedRewards);

  GridEvent gridEvent;
  gridEvent.actionName = "action";
  gridEvent.playerId = 2;
  gridEvent.sourceObjectPlayerId = 2;
  gridEvent.destinationObjectPlayerId = 2;
  gridEvent.sourceObjectName = "srcObject";
  gridEvent.destObjectName = "dstObject";
  gridEvent.sourceLocation = mockSourceObjectLocation;
  gridEvent.destLocation = mockDestinationObjectLocation;
  gridEvent.rewards = {{playerId, 11}};
  gridEvent.tick = 10;
  gridEvent.delay = 10;

  ASSERT_THAT(grid->getHistory(), ElementsAre(ActionEventMatcher(gridEvent)));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSourceObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(GridTest, objectCounters) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::shared_ptr<Object>>> objects;

  std::string objectName = "cat";
  grid->initObject("cat", {});
  for (uint32_t p = 0; p < 10; p++) {
    for (uint32_t o = 0; o < 5; o++) {
      auto mockObject = std::shared_ptr<MockObject>(new MockObject());

      glm::ivec2 location = {(int32_t)p, (int32_t)o};
      EXPECT_CALL(*mockObject, init).Times(1);
      EXPECT_CALL(*mockObject, getZIdx).WillRepeatedly(Return(0));
      EXPECT_CALL(*mockObject, getLocation).WillRepeatedly(Return(location));

      EXPECT_CALL(*mockObject, getPlayerId())
          .WillRepeatedly(Return(p));

      EXPECT_CALL(*mockObject, getObjectName())
          .WillRepeatedly(Return(objectName));

      grid->addObject(location, mockObject);

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

  grid->initObject("object", {});
  auto objectCounter = grid->getObjectCounter("object");

  ASSERT_EQ(*objectCounter[0], 0);
}

TEST(GridTest, runInitialActionsForObject) {
  auto grid = std::shared_ptr<Grid>(new Grid());
  grid->resetMap(123, 456);

  grid->initObject("object", {});
  auto mockObjectPtr = mockObject("object");
  auto mockActionPtr1 = std::shared_ptr<MockAction>(new MockAction());
  auto mockActionPtr2 = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr1, getSourceObject())
      .Times(1)
      .WillOnce(Return(mockObjectPtr));

  EXPECT_CALL(*mockActionPtr1, getDestinationObject())
      .Times(1)
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*mockActionPtr2, getSourceObject())
      .Times(1)
      .WillOnce(Return(mockObjectPtr));

  EXPECT_CALL(*mockActionPtr2, getDestinationObject())
      .Times(1)
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*mockObjectPtr, getInitialActions())
      .Times(1)
      .WillOnce(Return(std::vector<std::shared_ptr<Action>>{mockActionPtr1, mockActionPtr2}));

  grid->addObject({1, 2}, mockObjectPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr2.get()));
}

}  // namespace griddly