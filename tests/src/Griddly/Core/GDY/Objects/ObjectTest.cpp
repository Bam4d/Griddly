#include <unordered_map>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

TEST(ObjectTest, getLocation) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {}, nullptr));

  object->init(1, {5, 5}, mockGridPtr);

  ASSERT_EQ(object->getLocation(), GridLocation(5, 5));

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
  auto object = std::shared_ptr<Object>(new Object("object", 0, 0, {{"test_param", std::make_shared<int32_t>(20)}}, nullptr));

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

CommandTestResult dstCommandTest(std::string commandName, std::vector<std::string> params, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, initialVariables, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  dstObject->addActionDstBehaviour("action", srcObjectName, commandName, params, {});

  auto behaviourResult = dstObject->onActionDst(srcObject, mockActionPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));

  return {behaviourResult, srcObject, dstObject};
}

CommandTestResult srcCommandTest(std::string commandName, std::vector<std::string> params, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, initialVariables, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, commandName, params, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));

  return {behaviourResult, srcObject, dstObject};
}

TEST(ObjectTest, src_command_reward) {
  auto result = srcCommandTest("reward", {"10"}, {});

  auto behaviourResult = result.behaviourResult;

  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 10);
}

TEST(ObjectTest, src_command_override) {
  auto result1 = srcCommandTest("override", {"true", "123"}, {});
  auto behaviourResult1 = result1.behaviourResult;
  ASSERT_TRUE(behaviourResult1.abortAction);
  ASSERT_EQ(behaviourResult1.reward, 123);

  auto result2 = srcCommandTest("override", {"false", "-123"}, {});
  auto behaviourResult2 = result2.behaviourResult;
  ASSERT_FALSE(behaviourResult2.abortAction);
  ASSERT_EQ(behaviourResult2.reward, -123);
}

TEST(ObjectTest, src_command_incr) {
  auto result = srcCommandTest("incr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.srcObject->getVariableValue("test_param"), 21);
}

TEST(ObjectTest, src_command_decr) {
  auto result = srcCommandTest("decr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.srcObject->getVariableValue("test_param"), 19);
}

TEST(ObjectTest, dst_command_reward) {
  auto result = dstCommandTest("reward", {"10"}, {});

  auto behaviourResult = result.behaviourResult;

  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 10);
}

TEST(ObjectTest, dst_command_override) {
  auto result1 = dstCommandTest("override", {"true", "123"}, {});
  auto behaviourResult1 = result1.behaviourResult;
  ASSERT_TRUE(behaviourResult1.abortAction);
  ASSERT_EQ(behaviourResult1.reward, 123);

  auto result2 = dstCommandTest("override", {"false", "-123"}, {});
  auto behaviourResult2 = result2.behaviourResult;
  ASSERT_FALSE(behaviourResult2.abortAction);
  ASSERT_EQ(behaviourResult2.reward, -123);
}

TEST(ObjectTest, dst_command_incr) {
  auto result = dstCommandTest("incr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.dstObject->getVariableValue("test_param"), 21);
}

TEST(ObjectTest, dst_command_decr) {
  auto result = dstCommandTest("decr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.dstObject->getVariableValue("test_param"), 19);
}

TEST(ObjectTest, src_command_mov_action_dest) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto srcObjectStartLocation = GridLocation(3, 3);

  auto actionDestination = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation(Eq(srcObject)))
      .Times(1)
      .WillOnce(Return(actionDestination));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, srcObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(srcObject), Eq(srcObjectStartLocation), Eq(actionDestination)))
      .Times(1)
      .WillOnce(Return(true));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "mov", {"_dest"}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(srcObject->getLocation(), actionDestination);

  ASSERT_EQ(*srcObject->getVariableValue("_x"), 4);
  ASSERT_EQ(*srcObject->getVariableValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_mov_action_src) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto srcObjectStartLocation = GridLocation(3, 3);

  auto actionSource = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(actionSource));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, srcObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(srcObject), Eq(srcObjectStartLocation), Eq(actionSource)))
      .Times(1)
      .WillOnce(Return(true));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "mov", {"_src"}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(srcObject->getLocation(), actionSource);

  ASSERT_EQ(*srcObject->getVariableValue("_x"), 4);
  ASSERT_EQ(*srcObject->getVariableValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_mov_action_params) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";

  std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables;
  initialVariables.insert({"mov_x", std::make_shared<int32_t>(7)});
  initialVariables.insert({"mov_y", std::make_shared<int32_t>(12)});

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, initialVariables, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto srcObjectStartLocation = GridLocation(3, 3);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, srcObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(srcObject), Eq(srcObjectStartLocation), Eq(GridLocation(7, 12))))
      .Times(1)
      .WillOnce(Return(true));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "mov", {"mov_x", "mov_y"}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(srcObject->getLocation(), GridLocation(7, 12));

  ASSERT_EQ(*srcObject->getVariableValue("_x"), 7);
  ASSERT_EQ(*srcObject->getVariableValue("_y"), 12);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_dest) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto dstObjectStartLocation = GridLocation(3, 3);

  auto actionDestination = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation(Eq(dstObject)))
      .Times(1)
      .WillOnce(Return(actionDestination));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  dstObject->init(1, dstObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(dstObject), Eq(dstObjectStartLocation), Eq(actionDestination)))
      .Times(1)
      .WillOnce(Return(true));

  dstObject->addActionDstBehaviour("action", srcObjectName, "mov", {"_dest"}, {});

  auto behaviourResult = dstObject->onActionDst(srcObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(dstObject->getLocation(), actionDestination);

  ASSERT_EQ(*dstObject->getVariableValue("_x"), 4);
  ASSERT_EQ(*dstObject->getVariableValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_src) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto dstObjectStartLocation = GridLocation(3, 3);

  auto actionSource = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .Times(1)
      .WillOnce(Return(actionSource));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  dstObject->init(1, dstObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(dstObject), Eq(dstObjectStartLocation), Eq(actionSource)))
      .Times(1)
      .WillOnce(Return(true));

  dstObject->addActionDstBehaviour("action", srcObjectName, "mov", {"_src"}, {});

  auto behaviourResult = dstObject->onActionDst(srcObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(dstObject->getLocation(), actionSource);

  ASSERT_EQ(*dstObject->getVariableValue("_x"), 4);
  ASSERT_EQ(*dstObject->getVariableValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_params) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";

  std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables;
  initialVariables.insert({"mov_x", std::make_shared<int32_t>(7)});
  initialVariables.insert({"mov_y", std::make_shared<int32_t>(12)});

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, initialVariables, nullptr));

  auto dstObjectStartLocation = GridLocation(3, 3);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  dstObject->init(1, dstObjectStartLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, updateLocation(Eq(dstObject), Eq(dstObjectStartLocation), Eq(GridLocation(7, 12))))
      .Times(1)
      .WillOnce(Return(true));

  dstObject->addActionDstBehaviour("action", srcObjectName, "mov", {"mov_x", "mov_y"}, {});

  auto behaviourResult = dstObject->onActionDst(srcObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(dstObject->getLocation(), GridLocation(7, 12));

  ASSERT_EQ(*dstObject->getVariableValue("_x"), 7);
  ASSERT_EQ(*dstObject->getVariableValue("_y"), 12);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_cascade) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto srcObjectLocation = GridLocation(0, 0);
  auto dstObjectLocation = GridLocation(1, 0);
  auto cascadedObjectLocation = GridLocation(2, 0);

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, srcObjectLocation, mockGridPtr);
  dstObject->init(0, dstObjectLocation, mockGridPtr);

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(2)
      .WillRepeatedly(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation(Eq(srcObject)))
      .Times(1)
      .WillOnce(Return(dstObjectLocation));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "cascade", {"_dest"}, {});
  dstObject->addActionSrcBehaviour("action", "_empty", "nop", {}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_remove) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, {0, 0}, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, removeObject(Eq(srcObject)))
      .Times(1)
      .WillOnce(Return(true));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "remove", {}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_change_to) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto dstChangeObjectName = "dstChangeObject";

  auto mockObjectGenerator = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {}, mockObjectGenerator));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, mockObjectGenerator));
  auto dstChangeObject = std::shared_ptr<Object>(new Object(dstChangeObjectName, 0, 0, {}, mockObjectGenerator));

  auto dstObjectLocation = GridLocation{0, 1};
  auto dstObjectPlayerId = 2;

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  dstObject->init(dstObjectPlayerId, dstObjectLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, removeObject(Eq(dstObject)))
      .Times(1)
      .WillOnce(Return(true));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq(dstChangeObjectName), _))
      .Times(1)
      .WillOnce(Return(dstChangeObject));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(dstObjectPlayerId), Eq(dstObjectLocation), Eq(dstChangeObject)))
      .Times(1);

  dstObject->addActionDstBehaviour("action", dstObjectName, "change_to", {dstChangeObjectName}, {});

  auto behaviourResult = dstObject->onActionDst(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGenerator.get()));
}

TEST(ObjectTest, src_command_eq) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"resource", std::make_shared<int32_t>(0)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands1;
  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands2;

  conditionalCommands1["incr"] = {"resource"};
  conditionalCommands2["decr"] = {"resource"};

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "eq", {"0", "resource"}, conditionalCommands1);
  srcObject->addActionSrcBehaviour("action", dstObjectName, "eq", {"1", "resource"}, conditionalCommands2);

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("resource"), 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_lt) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", std::make_shared<int32_t>(0)}, {"small", std::make_shared<int32_t>(-100)}, {"big", std::make_shared<int32_t>(100)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands1;
  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands2;

  conditionalCommands1["incr"] = {"counter"};
  conditionalCommands2["incr"] = {"counter"};

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "lt", {"small", "0"}, conditionalCommands1);
  srcObject->addActionSrcBehaviour("action", dstObjectName, "lt", {"0", "big"}, conditionalCommands2);

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_gt) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", std::make_shared<int32_t>(0)}, {"small", std::make_shared<int32_t>(-100)}, {"big", std::make_shared<int32_t>(100)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands1;
  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands2;

  conditionalCommands1["incr"] = {"counter"};
  conditionalCommands2["incr"] = {"counter"};

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "gt", {"0", "small"}, conditionalCommands1);
  srcObject->addActionSrcBehaviour("action", dstObjectName, "gt", {"big", "0"}, conditionalCommands2);

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, checkPrecondition) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", std::make_shared<int32_t>(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition(actionName, dstObjectName, "eq", {"counter", "5"});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});
  
  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  // preconditions should come back as true because the counter value is equal to 5
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, checkPreconditionNotDefinedForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", std::make_shared<int32_t>(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition("different_action", dstObjectName, "eq", {"counter", "5"});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, checkPreconditionNotDefinedForDestination) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto actionName = "action";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, 0, {{"counter", std::make_shared<int32_t>(5)}}, nullptr));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, 0, {}, nullptr));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return(actionName));

  srcObject->addPrecondition(actionName, "different_destination_object", "eq", {"counter", "5"});
  srcObject->addActionSrcBehaviour(actionName, dstObjectName, "nop", {}, {});

  auto preconditionResult = srcObject->checkPreconditions(dstObject, mockActionPtr);

  // we add one to the resource and then decrement one from it if its equal to 1
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

}  // namespace griddly