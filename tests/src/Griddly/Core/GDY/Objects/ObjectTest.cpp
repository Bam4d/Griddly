#include <unordered_map>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define _V(X) std::make_shared<int32_t>(X)
#define _Y(X) YAML::Node(X)

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

std::shared_ptr<MockAction> setupAction(std::string actionName, uint32_t originatingPlayerId, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(actionName));

  EXPECT_CALL(*mockActionPtr, getSourceObject())
      .WillRepeatedly(Return(sourceObject));

  EXPECT_CALL(*mockActionPtr, getDestinationObject())
      .WillRepeatedly(Return(destObject));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(sourceObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(destObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getOriginatingPlayerId())
      .WillRepeatedly(Return(originatingPlayerId));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(destObject->getLocation() - sourceObject->getLocation()));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(actionName));

  EXPECT_CALL(*mockActionPtr, getSourceObject())
      .WillRepeatedly(Return(sourceObject));

  EXPECT_CALL(*mockActionPtr, getDestinationObject())
      .WillRepeatedly(Return(destObject));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(sourceObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(destObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getOriginatingPlayerId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(destObject->getLocation() - sourceObject->getLocation()));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(actionName));

  EXPECT_CALL(*mockActionPtr, getSourceObject())
      .WillRepeatedly(Return(sourceObject));

  EXPECT_CALL(*mockActionPtr, getDestinationObject())
      .WillRepeatedly(Return(nullptr));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(sourceObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(destLocation));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(destLocation - sourceObject->getLocation()));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(actionName));

  EXPECT_CALL(*mockActionPtr, getSourceObject())
      .WillRepeatedly(Return(sourceObject));

  EXPECT_CALL(*mockActionPtr, getDestinationObject())
      .WillRepeatedly(Return(destObject));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(sourceObject->getLocation()));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(destLocation));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(destLocation - sourceObject->getLocation()));

  return mockActionPtr;
}

TEST(ObjectTest, getLocation) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 'o', 0, 0, {}, nullptr));

  object->init({5, 5}, mockGridPtr);

  ASSERT_EQ(object->getLocation(), glm::ivec2(5, 5));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectName) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 'o', 0, 0, {}, nullptr));

  ASSERT_EQ(object->getObjectName(), "object");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getDescription) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 'o', 0, 0, {}, nullptr));

  object->init({9, 6}, mockGridPtr);

  ASSERT_EQ(object->getDescription(), "object@[9, 6]");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getPlayerId) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 'o', 2, 0, {}, nullptr));

  object->init({5, 5}, mockGridPtr);

  ASSERT_EQ(object->getPlayerId(), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getVariables) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 'o', 2, 0, {{"test_param", _V(20)}}, nullptr));

  ASSERT_EQ(*object->getVariableValue("test_param"), 20);

  object->init({5, 6}, mockGridPtr);

  ASSERT_EQ(*object->getVariableValue("_x"), 5);
  ASSERT_EQ(*object->getVariableValue("_y"), 6);

  ASSERT_EQ(object->getVariableValue("does_not_exist"), nullptr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, actionBoundToSrc) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  srcObject->addActionSrcBehaviour("action", dstObjectName, "nop", {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr);

  ASSERT_FALSE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, actionBoundToDst) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  dstObject->addActionDstBehaviour("action", srcObjectName, "nop", {}, {});

  auto dstResult = dstObject->onActionDst(mockActionPtr);

  ASSERT_FALSE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// Test that if the destination object returns _empty/is nullptr then we still perform source action commands based on the dstObjectName
// This can be the case when destination objects are removed during a behaviour
TEST(ObjectTest, actionDestinationObjectDifferentFromOriginalObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, glm::ivec2{1, 1});

  srcObject->addActionSrcBehaviour("action", dstObjectName, "nop", {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr);

  ASSERT_FALSE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is registered for dst object and action, but not performed on different dst object
TEST(ObjectTest, srcActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  srcObject->addActionSrcBehaviour("action", "not_dst_object", "nop", {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr);

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, srcActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr);

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dstActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  dstObject->addActionDstBehaviour("action", "not_src_object", "nop", {}, {});

  auto dstResult = dstObject->onActionDst(mockActionPtr);

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, dstActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction("action", srcObject, dstObject);

  auto dstResult = dstObject->onActionDst(mockActionPtr);

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

struct CommandTestResult {
  BehaviourResult behaviourResult;
  std::shared_ptr<Object> srcObject;
  std::shared_ptr<Object> dstObject;
};

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, glm::ivec2 location, DiscreteOrientation orientation, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr, std::shared_ptr<ObjectGenerator> objectGenerator) {
  auto object = std::shared_ptr<Object>(new Object(objectname, 'o', playerId, 0, initialVariables, objectGenerator));

  if (mockGridPtr != nullptr) {
    object->init(location, orientation, mockGridPtr);

    EXPECT_CALL(*mockGridPtr, getObject(Eq(location)))
        .WillRepeatedly(Return(object));
  }

  return object;
}

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, glm::ivec2 location, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr) {
  return setupObject(playerId, objectname, location, DiscreteOrientation(), initialVariables, mockGridPtr, nullptr);
}

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, glm::ivec2 location, DiscreteOrientation orientation, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr) {
  return setupObject(playerId, objectname, location, orientation, initialVariables, mockGridPtr, nullptr);
}

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables) {
  return setupObject(playerId, objectname, {0, 0}, initialVariables, nullptr);
}

std::shared_ptr<Object> setupObject(std::string objectname, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables) {
  return setupObject(1, objectname, {0, 0}, initialVariables, nullptr);
}

std::shared_ptr<Object> setupObject(std::string objectname, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr) {
  return setupObject(1, objectname, {0, 0}, initialVariables, mockGridPtr);
}

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, BehaviourCommandArguments commandArgumentMap, CommandList conditionalCommands, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  switch (type) {
    case ActionBehaviourType::DESTINATION: {
      dstObjectPtr->addActionDstBehaviour(action->getActionName(), srcObjectPtr->getObjectName(), commandName, commandArgumentMap, conditionalCommands);
      return dstObjectPtr->onActionDst(action);
    }
    case ActionBehaviourType::SOURCE: {
      auto dstObjectName = dstObjectPtr == nullptr ? "_empty" : dstObjectPtr->getObjectName();
      srcObjectPtr->addActionSrcBehaviour(action->getActionName(), dstObjectName, commandName, commandArgumentMap, conditionalCommands);
      return srcObjectPtr->onActionSrc(dstObjectName, action);
    }
  }

  return {true};
}

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, BehaviourCommandArguments commandArgumentMap, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  return addCommandsAndExecute(type, action, commandName, commandArgumentMap, {}, srcObjectPtr, dstObjectPtr);
}

std::shared_ptr<MockGrid> mockGrid() {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  EXPECT_CALL(*mockGridPtr, updateLocation)
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*mockGridPtr, invalidateLocation)
      .WillRepeatedly(Return(true));

  return mockGridPtr;
}

void verifyCommandResult(BehaviourResult result, bool abort, std::unordered_map<uint32_t, int32_t> rewards) {
  ASSERT_EQ(result.abortAction, abort);
  ASSERT_EQ(result.rewards, rewards);
}

void verifyMocks(std::shared_ptr<MockAction> mockActionPtr, std::shared_ptr<MockGrid> mockGridPtr = nullptr, std::shared_ptr<MockObjectGenerator> mockObjectGenerator = nullptr) {
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));

  if (mockGridPtr != nullptr) {
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  }

  if (mockObjectGenerator != nullptr) {
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGenerator.get()));
  }
}

MATCHER_P2(ActionListMatcher, actionName, numActions, "") {
  return arg.size() == numActions && actionName == arg[0]->getActionName();
}

MATCHER_P4(SingletonDelayedActionVectorMatcher, actionName, delay, sourceObjectPtr, vectorToDest, "") {
  auto action = arg[0];
  return arg.size() == 1 &&
         action->getDelay() == 10 &&
         action->getActionName() == actionName &&
         action->getSourceObject().get() == sourceObjectPtr.get() &&
         action->getVectorToDest() == vectorToDest;
}

MATCHER_P3(SingletonMappedToGridMatcher, actionName, sourceObjectPtr, destinationLocationRange, "") {
  auto action = arg[0];
  return arg.size() == 1 &&
         action->getActionName() == actionName &&
         action->getSourceObject().get() == sourceObjectPtr.get() &&
         action->getDestinationLocation().x < destinationLocationRange.x &&
         action->getDestinationLocation().y < destinationLocationRange.y;
}

MATCHER_P4(SingletonActionVectorOriginatingPlayerMatcher, actionName, sourceObjectPtr, originatingPlayerId, vectorToDest, "") {
  auto action = arg[0];
  return arg.size() == 1 &&
         action->getActionName() == actionName &&
         action->getSourceObject().get() == sourceObjectPtr.get() &&
         action->getVectorToDest() == vectorToDest &&
         action->getOriginatingPlayerId() == originatingPlayerId;
}

TEST(ObjectTest, command_reward) {
  auto srcObjectPtr = setupObject(1, "srcObject", {});
  auto dstObjectPtr = setupObject(3, "dstObject", {});
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("10")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("-10")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 10}});
  verifyCommandResult(dstResult, false, {{3, -10}});

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_reward_variable) {
  auto srcObjectPtr = setupObject(1, "srcObject", {{"ten", _V(10)}});
  auto dstObjectPtr = setupObject(3, "dstObject", {{"minusten", _V(-10)}});
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("ten")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("minusten")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 10}});
  verifyCommandResult(dstResult, false, {{3, -10}});

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_reward_default_to_action_player_id) {
  auto srcObjectPtr = setupObject(0, "srcObject", {});
  auto dstObjectPtr = setupObject(0, "dstObject", {});
  auto mockActionPtr = setupAction("action", 2, srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("10")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("-10")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{2, 10}});
  verifyCommandResult(dstResult, false, {{2, -10}});

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_set) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "set", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "set", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 5);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 5);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_add) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "add", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "add", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 25);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 25);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_sub) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "sub", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "sub", {{"0", _Y("test_param")}, {"1", _Y("5")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 15);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 15);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_incr) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "incr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "incr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 21);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 21);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_decr) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _V(20)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "decr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "decr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 19);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 19);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_mov_dest) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - mov: _dest
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - mov: _src

  auto mockGridPtr = mockGrid();

  auto actionSource = glm::ivec2(3, 3);
  auto actionDestination = glm::ivec2(4, 3);

  auto srcObjectPtr = setupObject(1, "srcObject", actionSource, {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", actionDestination, {}, mockGridPtr);

  auto mockActionPtr = setupAction("move", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "mov", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "mov", {{"0", _Y("_src")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(srcObjectPtr->getLocation(), actionDestination);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_x"), 4);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_y"), 3);

  ASSERT_EQ(dstObjectPtr->getLocation(), actionSource);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_x"), 3);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_y"), 3);

  verifyMocks(mockActionPtr, mockGridPtr);
}

// ! This tests that we can do actions like pulling
TEST(ObjectTest, command_mov_action_src) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - mov: _src
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - mov: _src

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(2, 3), {}, mockGridPtr);

  auto mockActionPtr = setupAction("move", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "mov", {{"0", _Y("_src")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "mov", {{"0", _Y("_src")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(srcObjectPtr->getLocation(), glm::ivec2(3, 3));
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_x"), 3);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_y"), 3);

  ASSERT_EQ(dstObjectPtr->getLocation(), glm::ivec2(3, 3));
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_x"), 3);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_y"), 3);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_mov_action_params) {
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), {{"mov_x", _V(7)}, {"mov_y", _V(12)}}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(2, 3), {{"mov_x", _V(8)}, {"mov_y", _V(10)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "mov", {{"0", _Y("mov_x")}, {"1", _Y("mov_y")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "mov", {{"0", _Y("mov_x")}, {"1", _Y("mov_y")}},
                                         srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(srcObjectPtr->getLocation(), glm::ivec2(7, 12));
  ASSERT_EQ(dstObjectPtr->getLocation(), glm::ivec2(8, 10));

  ASSERT_EQ(*srcObjectPtr->getVariableValue("_x"), 7);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_y"), 12);

  ASSERT_EQ(*dstObjectPtr->getVariableValue("_x"), 8);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_y"), 10);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_cascade) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - cascade: _dest
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - cascade: _dest

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(3, "dstObject", glm::ivec2(1, 0), {}, mockGridPtr);

  auto mockActionPtr1 = setupAction("action1", srcObjectPtr, dstObjectPtr);
  auto mockActionPtr2 = setupAction("action2", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), ActionListMatcher("action1", 1)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 1}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), ActionListMatcher("action2", 1)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{3, 2}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr1, "cascade", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr2, "cascade", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 1}});
  verifyCommandResult(dstResult, false, {{3, 2}});

  verifyMocks(mockActionPtr1, mockGridPtr);
  verifyMocks(mockActionPtr2);
}

TEST(ObjectTest, command_mapped_to_grid) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: mapped_to_grid
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: mapped_to_grid
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(2, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(5, "dstObject", glm::ivec2(1, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"mapped_to_grid", {{}, false, true, true}}};

  int maxHeight = 10;
  int maxWidth = 100;

  auto gridDimensions = glm::ivec2(maxWidth, maxHeight);

  EXPECT_CALL(*mockGridPtr, getWidth())
      .WillRepeatedly(Return(maxWidth));

  EXPECT_CALL(*mockGridPtr, getHeight())
      .WillRepeatedly(Return(maxHeight));

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonMappedToGridMatcher("mapped_to_grid", srcObjectPtr, gridDimensions)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonMappedToGridMatcher("mapped_to_grid", dstObjectPtr, gridDimensions)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{5, 3}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("mapped_to_grid")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("mapped_to_grid")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{2, 3}});
  verifyCommandResult(dstResult, false, {{5, 3}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_delayed) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: 10
  //*           ActionId: 2
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: 10
  //*           ActionId: 2
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(10, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(2, "dstObject", glm::ivec2(1, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                           {2, {{0, -1}, {0, -1}, "Up"}},
                           {3, {{1, 0}, {1, 0}, "Right"}},
                           {4, {{0, 1}, {0, 1}, "Down"}},
                       },
                       false,
                       false}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, srcObjectPtr, glm::ivec2(0, -1))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{10, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, dstObjectPtr, glm::ivec2(0, -1))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 6}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Delay", _Y("10")}, {"ActionId", _Y(2)}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Delay", _Y("10")}, {"ActionId", _Y(2)}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{10, 3}});
  verifyCommandResult(dstResult, false, {{2, 6}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(2, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(10, "dstObject", glm::ivec2(1, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                           {2, {{0, -1}, {0, -1}, "Up"}},
                           {3, {{1, 0}, {1, 0}, "Right"}},
                           {4, {{0, 1}, {0, 1}, "Down"}},
                       },
                       false,
                       false}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 1, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 1, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{10, 6}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{2, 3}});
  verifyCommandResult(dstResult, false, {{10, 6}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_with_action_player_id) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*           Executor: action
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*           Executor: action
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(2, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(10, "dstObject", glm::ivec2(1, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                           {2, {{0, -1}, {0, -1}, "Up"}},
                           {3, {{1, 0}, {1, 0}, "Right"}},
                           {4, {{0, 1}, {0, 1}, "Down"}},
                       },
                       false,
                       false}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  auto mockActionPtr1 = setupAction("do_exec", 5, srcObjectPtr, dstObjectPtr);
  auto mockActionPtr2 = setupAction("do_exec", 4, srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 5, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 4, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{10, 6}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr1, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y("action")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr2, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y("action")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{2, 3}});
  verifyCommandResult(dstResult, false, {{10, 6}});

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr2.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, command_exec_with_object_player_id) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*           Executor: object
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: 2
  //*           Executor: object
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(2, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(10, "dstObject", glm::ivec2(1, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                           {2, {{0, -1}, {0, -1}, "Up"}},
                           {3, {{1, 0}, {1, 0}, "Right"}},
                           {4, {{0, 1}, {0, 1}, "Down"}},
                       },
                       false,
                       false}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 2, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 10, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{10, 6}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y("object")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y("object")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{2, 3}});
  verifyCommandResult(dstResult, false, {{10, 6}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_randomize) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Randomize: true
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Randomize: true
  //*
  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                       },
                       false,
                       false}}};

  srand(100);

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(Return(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 1, glm::ivec2(-1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 1, glm::ivec2(-1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Randomize", _Y(true)}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Randomize", _Y(true)}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 3}});
  verifyCommandResult(dstResult, false, {{1, 3}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_remove) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - remove: true
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*      - remove: true
  //*

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(0, "dstObject", glm::ivec2(1, 0), {}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(srcObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(dstObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "remove", {}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "remove", {}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_change_to) {
  //* - Src:
  //*    Object: srcObject
  //*    Commands:
  //*      - change_to: newObject
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*      - change_to: newObject
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(2, "dstObject", glm::ivec2(1, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto globalVariables = std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>{
      {"global_variable", {{0, _V(10)}}}};

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, getGlobalVariables)
      .WillRepeatedly(ReturnRef(globalVariables));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(1), Eq(globalVariables)))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(2), Eq(globalVariables)))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(srcObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(0, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(dstObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(1, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr)))
      .Times(1);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "change_to", {{"0", _Y("newObject")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "change_to", {{"0", _Y("newObject")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  verifyMocks(mockActionPtr, mockGridPtr, mockObjectGenerator);
}

TEST(ObjectTest, command_set_tile) {
  //* - Src:
  //*    Object: srcObject
  //*    Commands:
  //*      - set_tile: 1
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*      - set_tile: 1
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(2, "dstObject", glm::ivec2(1, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  ASSERT_EQ(srcObjectPtr->getObjectRenderTileName(), "srcObject0");
  ASSERT_EQ(dstObjectPtr->getObjectRenderTileName(), "dstObject0");

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "set_tile", {{"0", _Y(1)}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "set_tile", {{"0", _Y(1)}}, srcObjectPtr, dstObjectPtr);

  ASSERT_EQ(srcObjectPtr->getObjectRenderTileName(), "srcObject1");
  ASSERT_EQ(dstObjectPtr->getObjectRenderTileName(), "dstObject1");

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  verifyMocks(mockActionPtr, mockGridPtr, mockObjectGenerator);
}

TEST(ObjectTest, command_spawn) {
  //* - Src:
  //*    Object: srcObject
  //*    Commands:
  //*      - spawn: newObject
  //*   Dst:
  //*     Object: _empty
  //*

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto mockActionPtr = setupAction("action", srcObjectPtr, glm::ivec2(1, 0));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(1), _))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(1, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr)))
      .Times(1);

  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables)
      .WillOnce(ReturnRef(globalVariables));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "spawn", {{"0", _Y("newObject")}}, srcObjectPtr, nullptr);

  verifyCommandResult(srcResult, false, {});

  verifyMocks(mockActionPtr, mockGridPtr, mockObjectGenerator);
}

TEST(ObjectTest, command_eq) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [resource, 0]
  //*           Commands:
  //*             - incr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [resource, 1]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(0)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(1)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "eq", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "eq", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 1);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_eq_src_dst_qualifiers) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [dst.resource, 0]
  //*           Commands:
  //*             - incr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [src.resource, 1]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(0)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(1)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "eq", {{"0", _Y("dst.resource")}, {"1", _Y("1")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "eq", {{"0", _Y("src.resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 1);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_eq_meta_qualifiers) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [meta.test_src, 0]
  //*           Commands:
  //*             - decr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - eq:
  //*           Arguments: [meta.test_dst, 1]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(1)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(1)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockActionPtr, getMetaData(Eq("test_src")))
      .WillRepeatedly(Return(0));

  EXPECT_CALL(*mockActionPtr, getMetaData(Eq("test_dst")))
      .WillRepeatedly(Return(1));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "eq", {{"0", _Y("meta.test_src")}, {"1", _Y("0")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "eq", {{"0", _Y("meta.test_dst")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 0);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_lt) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - lt:
  //*           Arguments: [resource, 1]
  //*           Commands:
  //*             - incr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - lt:
  //*           Arguments: [resource, 2]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(0)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(1)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "lt", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "lt", {{"0", _Y("resource")}, {"1", _Y("2")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 1);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_gt) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - gt:
  //*           Arguments: [resource, 0]
  //*           Commands:
  //*             - incr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - gt:
  //*           Arguments: [resource, 2]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(1)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(2)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "gt", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "gt", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 2);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 1);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_neq) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - neq:
  //*           Arguments: [resource, 10]
  //*           Commands:
  //*             - incr: resource
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - neq:
  //*           Arguments: [resource, 12]
  //*           Commands:
  //*             - decr: resource

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject("srcObject", {{"resource", _V(1)}}, mockGridPtr);
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _V(2)}}, mockGridPtr);

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "neq", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "neq", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 2);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 1);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, isValidAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);
  // auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  // EXPECT_CALL(*mockActionPtr, getActionName())
  //     .Times(1)
  //     .WillOnce(Return(actionName));

  srcObject->addPrecondition(actionName, dstObjectName, "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->isValidAction(mockActionPtr);

  // preconditions should come back as true because the counter value is equal to 5
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);
}

TEST(ObjectTest, isValidActionNotDefinedForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  srcObject->addPrecondition("different_action", dstObjectName, "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->isValidAction(mockActionPtr);

  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, isValidActionNotDefinedForDestination) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 'D', 0, 0, {}, nullptr));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  srcObject->addPrecondition(actionName, "different_destination_object", "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->isValidAction(mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, isValidActionDestinationLocationOutsideGrid) {
  auto srcObjectName = "srcObject";
  auto dstEmptyObjectName = "_empty";
  auto dstBoundaryObjectName = "_boundary";
  auto actionName = "action";

  auto mockGridPtr = mockGrid();

  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(10));
  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(10));

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr));

  auto dstObjectOutside = std::shared_ptr<Object>(new Object(dstBoundaryObjectName, 'S', 0, 0, {}, nullptr));
  auto dstObjectInside = std::shared_ptr<Object>(new Object(dstEmptyObjectName, 'D', 0, 0, {}, nullptr));

  srcObject->init({5, 4}, DiscreteOrientation(), mockGridPtr);

  dstObjectOutside->init({-1, -1}, DiscreteOrientation(), mockGridPtr);
  dstObjectInside->init({5, 5}, DiscreteOrientation(), mockGridPtr);

  auto mockActionPtrOutside = setupAction(actionName, srcObject, dstObjectOutside);
  auto mockActionPtrValid = setupAction(actionName, srcObject, dstObjectInside);

  srcObject->addActionSrcBehaviour(actionName, dstEmptyObjectName, "nop", {}, {});
  srcObject->addActionSrcBehaviour(actionName, dstBoundaryObjectName, "nop", {}, {});

  auto preconditionResultOutside = srcObject->isValidAction(mockActionPtrOutside);
  auto preconditionResultValid = srcObject->isValidAction(mockActionPtrValid);

  ASSERT_TRUE(preconditionResultOutside);
  ASSERT_TRUE(preconditionResultValid);

  verifyMocks(mockActionPtrOutside);
  verifyMocks(mockActionPtrValid);
}

TEST(ObjectTest, getInitialActions) {
  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());

  std::string objectName = "objectName";

  std::vector<InitialActionDefinition> initialActionDefinitions = {
      {
          "action1Name",
          1,
          0,
          false,
          1.0,
      },
      {
          "action2Name",
          1,
          0,
          true,
          1.0,
      }};

  std::unordered_map<std::string, ActionInputsDefinition> mockActionInputDefinitions = {
      {"action1Name",
       {{{1, {{1, 1}, {1, 1}, "description1"}}}}},
      {"action2Name",
       {{{2, {{2, 2}, {2, 2}, "description2"}}}}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions()).WillRepeatedly(Return(mockActionInputDefinitions));

  auto object = std::shared_ptr<Object>(new Object(objectName, 'S', 0, 0, {}, mockObjectGenerator));

  object->setInitialActionDefinitions(initialActionDefinitions);

  auto actions = object->getInitialActions(nullptr);

  ASSERT_EQ(actions.size(), 2);

  ASSERT_EQ(actions[0]->getVectorToDest(), glm::ivec2(1, 1));
  ASSERT_EQ(actions[0]->getOrientationVector(), glm::ivec2(1, 1));

  ASSERT_EQ(actions[1]->getVectorToDest(), glm::ivec2(2, 2));
  ASSERT_EQ(actions[1]->getOrientationVector(), glm::ivec2(2, 2));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGenerator.get()));
}

TEST(ObjectTest, getInitialActionsWithOriginatingAction) {
  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());

  std::string objectName = "objectName";
  std::string originatingActionName = "originatingAction";

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(originatingActionName));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(glm::ivec2{3, 3}));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(glm::ivec2{4, 4}));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(glm::ivec2{3, 1}));

  EXPECT_CALL(*mockActionPtr, getOrientationVector())
      .WillRepeatedly(Return(glm::ivec2{1, 5}));

  std::vector<InitialActionDefinition> initialActionDefinitions = {
      {
          "action1Name",
          0,
          0,
          false,
          1.0,
      },
      {
          "action2Name",
          1,
          0,
          true,
          1.0,
      }};

  std::unordered_map<std::string, ActionInputsDefinition> mockActionInputDefinitions = {
      {"action1Name",
       {{}}},
      {"action2Name",
       {{{2, {{2, 2}, {2, 2}, "description2"}}}}}};

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions()).WillRepeatedly(Return(mockActionInputDefinitions));

  auto object = std::shared_ptr<Object>(new Object(objectName, 'S', 0, 0, {}, mockObjectGenerator));

  object->setInitialActionDefinitions(initialActionDefinitions);

  auto actions = object->getInitialActions(mockActionPtr);

  ASSERT_EQ(actions.size(), 2);

  ASSERT_EQ(actions[0]->getVectorToDest(), glm::ivec2(3, 1));
  ASSERT_EQ(actions[0]->getOrientationVector(), glm::ivec2(1, 5));

  ASSERT_EQ(actions[1]->getVectorToDest(), glm::ivec2(2, 2));
  ASSERT_EQ(actions[1]->getOrientationVector(), glm::ivec2(2, 2));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGenerator.get()));
}

}  // namespace griddly