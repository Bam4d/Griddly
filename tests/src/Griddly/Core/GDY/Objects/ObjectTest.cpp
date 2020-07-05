#include <unordered_map>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define _P(X) std::make_shared<int32_t>(X)
#define _Y(X) YAML::Node(X)

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

TEST(ObjectTest, getLocation) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  object->init(1, {5, 5}, mockGridPtr);

  ASSERT_EQ(object->getLocation(), glm::ivec2(5, 5));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectName) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  ASSERT_EQ(object->getObjectName(), "object");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectId) {
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  ASSERT_EQ(object->getObjectId(), 0);
}

TEST(ObjectTest, getDescription) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  object->init(1, {9, 6}, mockGridPtr);

  ASSERT_EQ(object->getDescription(), "object@[9, 6]");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getPlayerId) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  object->init(2, {5, 5}, mockGridPtr);

  ASSERT_EQ(object->getPlayerId(), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getVariables) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {{"test_param", _P(20)}}, nullptr));

  ASSERT_EQ(*object->getVariableValue("test_param"), 20);

  object->init(2, {5, 6}, mockGridPtr);

  ASSERT_EQ(*object->getVariableValue("_x"), 5);
  ASSERT_EQ(*object->getVariableValue("_y"), 6);

  ASSERT_EQ(object->getVariableValue("does_not_exist"), nullptr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, actionBoundToSrc) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "nop", {}, {});

  auto srcResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  ASSERT_FALSE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, actionBoundToDst) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());

  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  dstObject->addActionDstBehaviour("action", srcObjectName, "nop", {}, {});

  auto dstResult = dstObject->onActionDst(srcObject, mockActionPtr);

  ASSERT_FALSE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is registered for dst object and action, but not performed on different dst object
TEST(ObjectTest, srcActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", "not_dst_object", "nop", {}, {});

  auto srcResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, srcActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  auto srcResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dstActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  dstObject->addActionDstBehaviour("action", "not_src_object", "nop", {}, {});

  auto dstResult = dstObject->onActionDst(srcObject, mockActionPtr);

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, dstActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  auto dstResult = dstObject->onActionDst(srcObject, mockActionPtr);

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

struct CommandTestResult {
  BehaviourResult behaviourResult;
  std::shared_ptr<Object> srcObject;
  std::shared_ptr<Object> dstObject;
};

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

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, glm::ivec2 location, DiscreteOrientation orientation, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr, std::shared_ptr<ObjectGenerator> objectGenerator) {
  auto object = std::shared_ptr<Object>(new Object(objectname, 0, 0, initialVariables, objectGenerator));

  if (mockGridPtr != nullptr) {
    object->init(playerId, location, orientation, mockGridPtr);

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

std::shared_ptr<Object> setupObject(std::string objectname, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables) {
  return setupObject(0, objectname, {0, 0}, initialVariables, nullptr);
}

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, BehaviourCommandArguments commandArgumentMap, std::unordered_map<std::string, BehaviourCommandArguments> conditionalCommands, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  switch (type) {
    case ActionBehaviourType::DESTINATION: {
      dstObjectPtr->addActionDstBehaviour(action->getActionName(), srcObjectPtr->getObjectName(), commandName, commandArgumentMap, conditionalCommands);
      return dstObjectPtr->onActionDst(srcObjectPtr, action);
    }
    case ActionBehaviourType::SOURCE: {
      srcObjectPtr->addActionSrcBehaviour(action->getActionName(), dstObjectPtr->getObjectName(), commandName, commandArgumentMap, conditionalCommands);
      return srcObjectPtr->onActionSrc(dstObjectPtr, action);
    }
  }
}

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, BehaviourCommandArguments commandArgumentMap, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  return addCommandsAndExecute(type, action, commandName, commandArgumentMap, {}, srcObjectPtr, dstObjectPtr);
}

std::shared_ptr<MockGrid> mockGrid() {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  EXPECT_CALL(*mockGridPtr, updateLocation)
      .WillRepeatedly(Return(true));

  return mockGridPtr;
}

void verifyCommandResult(BehaviourResult result, bool abort, int32_t reward) {
  ASSERT_EQ(result.abortAction, abort);
  ASSERT_EQ(result.reward, reward);
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
         action->getActionName() == actionName &&
         action->getSourceObject().get() == sourceObjectPtr.get() &&
         action->getVectorToDest() == vectorToDest;
}

MATCHER_P3(SingletonActionVectorMatcher, actionName, sourceObjectPtr, vectorToDest, "") {
  auto action = arg[0];
  return arg.size() == 1 &&
         action->getActionName() == actionName &&
         action->getSourceObject().get() == sourceObjectPtr.get() &&
         action->getVectorToDest() == vectorToDest;
}

TEST(ObjectTest, command_reward) {
  auto srcObjectPtr = setupObject("srcObject", {});
  auto dstObjectPtr = setupObject("dstObject", {});
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("10")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("10")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 10);
  verifyCommandResult(dstResult, false, 10);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_override) {
  auto srcObjectPtr = setupObject("srcObject", {});
  auto dstObjectPtr = setupObject("dstObject", {});
  auto mockActionPtr1 = setupAction("override_true", srcObjectPtr, dstObjectPtr);

  auto srcResult1 = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr1, "override", {{"0", _Y("true")}, {"1", _Y("123")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult1 = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr1, "override", {{"0", _Y("true")}, {"1", _Y("123")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult1, true, 123);
  verifyCommandResult(dstResult1, true, 123);

  auto mockActionPtr2 = setupAction("override_false", srcObjectPtr, dstObjectPtr);

  auto srcResult2 = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr2, "override", {{"0", _Y("false")}, {"1", _Y("-123")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult2 = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr2, "override", {{"0", _Y("false")}, {"1", _Y("-123")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult2, false, -123);
  verifyCommandResult(dstResult2, false, -123);

  verifyMocks(mockActionPtr1);
  verifyMocks(mockActionPtr2);
}

TEST(ObjectTest, command_incr) {
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _P(20)}});
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _P(20)}});
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "incr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "incr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 21);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 21);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_decr) {
  auto srcObjectPtr = setupObject("srcObject", {{"test_param", _P(20)}});
  auto dstObjectPtr = setupObject("dstObject", {{"test_param", _P(20)}});
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "decr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "decr", {{"0", _Y("test_param")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(*srcObjectPtr->getVariableValue("test_param"), 19);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("test_param"), 19);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_mov_dest) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - mov: _dest
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - mov: _dest

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(0, 3), {}, mockGridPtr);

  auto actionDestination = glm::ivec2(4, 3);
  auto mockActionPtr = setupAction("move", srcObjectPtr, actionDestination);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "mov", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "mov", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(srcObjectPtr->getLocation(), actionDestination);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_x"), 4);
  ASSERT_EQ(*srcObjectPtr->getVariableValue("_y"), 3);

  ASSERT_EQ(dstObjectPtr->getLocation(), actionDestination);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("_x"), 4);
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

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

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
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), {{"mov_x", _P(7)}, {"mov_y", _P(12)}}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(2, 3), {{"mov_x", _P(8)}, {"mov_y", _P(10)}}, mockGridPtr);
  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "mov", {{"0", _Y("mov_x")}, {"1", _Y("mov_y")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "mov", {{"0", _Y("mov_x")}, {"1", _Y("mov_y")}},
                                         srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

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
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(1, 0), {}, mockGridPtr);

  auto mockActionPtr1 = setupAction("action1", srcObjectPtr, dstObjectPtr);
  auto mockActionPtr2 = setupAction("action2", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), ActionListMatcher("action1", 1)))
      .Times(1)
      .WillOnce(Return(std::vector<int>{1}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), ActionListMatcher("action2", 1)))
      .Times(1)
      .WillOnce(Return(std::vector<int>{2}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr1, "cascade", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr2, "cascade", {{"0", _Y("_dest")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 1);
  verifyCommandResult(dstResult, false, 2);

  verifyMocks(mockActionPtr1, mockGridPtr);
  verifyMocks(mockActionPtr2);
}

TEST(ObjectTest, command_exec_delayed) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: 10
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: 10
  //*

  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(1, 0), {}, mockGridPtr);

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, srcObjectPtr, glm::ivec2(1, 0))))
      .Times(1)
      .WillRepeatedly(Return(std::vector<int>{3}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, dstObjectPtr, glm::ivec2(1, 0))))
      .Times(1)
      .WillRepeatedly(Return(std::vector<int>{3}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Delay", _Y("10")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Delay", _Y("10")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 3);
  verifyCommandResult(dstResult, false, 3);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           VectorToDest: [3, 2]
  //*           Relative: true
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           VectorToDest: [2, 3]
  //*           Relative: true
  //*
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(Direction::DOWN), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(Direction::RIGHT), {}, mockGridPtr);

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  auto srcVectorToDest = YAML::Node(std::vector{3, 2});
  auto dstVectorToDest = YAML::Node(std::vector{2, 3});

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorMatcher("exec_action", srcObjectPtr, glm::ivec2{3,2})))
      .Times(1)
      .WillOnce(Return(std::vector<int>{3}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorMatcher("exec_action", dstObjectPtr, glm::ivec2{2,3})))
      .Times(1)
      .WillOnce(Return(std::vector<int>{3}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"VectorToDest", srcVectorToDest}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"VectorToDest", dstVectorToDest}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 3);
  verifyCommandResult(dstResult, false, 3);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_relative) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           VectorToDest: [3, 2]
  //*           Relative: true
  //*
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           VectorToDest: [3, 2]
  //*           Relative: true
  //*
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(Direction::DOWN), {}, mockGridPtr);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(Direction::RIGHT), {}, mockGridPtr);

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorMatcher("exec_action", srcObjectPtr, glm::ivec2(-3, -2))))
      .Times(1)
      .WillOnce(Return(std::vector<int>{3}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorMatcher("exec_action", dstObjectPtr, glm::ivec2(-2, 3))))
      .Times(1)
      .WillOnce(Return(std::vector<int>{3}));

  auto srcVectorToDest = YAML::Node(std::vector{3, 2});
  auto dstVectorToDest = YAML::Node(std::vector{3, 2});

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"VectorToDest", srcVectorToDest}, {"Relative", _Y("true")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"VectorToDest", dstVectorToDest}, {"Relative", _Y("true")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 3);
  verifyCommandResult(dstResult, false, 3);

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

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

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

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), _))
      .Times(2)
      .WillRepeatedly(Return(newObjectPtr));

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(srcObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, initObject(Eq(1), Eq(glm::ivec2(0, 0)), Eq(newObjectPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(dstObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, initObject(Eq(2), Eq(glm::ivec2(1, 0)), Eq(newObjectPtr)))
      .Times(1);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "change_to", {{"0", _Y("newObject")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "change_to", {{"0", _Y("newObject")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

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

  auto srcObjectPtr = setupObject("srcObject", {{"resource", _P(0)}});
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _P(1)}});

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "eq", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "eq", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 1);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);

  verifyMocks(mockActionPtr);
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

  auto srcObjectPtr = setupObject("srcObject", {{"resource", _P(0)}});
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _P(1)}});

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "lt", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "lt", {{"0", _Y("resource")}, {"1", _Y("2")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 1);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 0);
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

  auto srcObjectPtr = setupObject("srcObject", {{"resource", _P(1)}});
  auto dstObjectPtr = setupObject("dstObject", {{"resource", _P(2)}});

  auto mockActionPtr = setupAction("action", srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "gt", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "gt", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, 0);
  verifyCommandResult(dstResult, false, 0);

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 2);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 1);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, checkPrecondition) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", _P(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition(actionName, dstObjectName, "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  // preconditions should come back as true because the counter value is equal to 5
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, checkPreconditionNotDefinedForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", _P(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition("different_action", dstObjectName, "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, checkPreconditionNotDefinedForDestination) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", _P(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition(actionName, "different_destination_object", "eq", {{"0", _Y("counter")}, {"1", _Y("5")}});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

}  // namespace griddly