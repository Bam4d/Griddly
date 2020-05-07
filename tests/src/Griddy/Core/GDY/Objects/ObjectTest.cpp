#include <unordered_map>

#include "Griddy/Core/GDY/Objects/Object.hpp"
#include "Mocks/Griddy/Core/GDY/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(ObjectTest, getLocation) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  object->init(1, {5, 5}, mockGridPtr);

  ASSERT_EQ(object->getLocation(), GridLocation(5, 5));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectName) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  ASSERT_EQ(object->getObjectName(), "object");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectId) {
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  ASSERT_EQ(object->getObjectId(), 0);
}

TEST(ObjectTest, getDescription) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  object->init(1, {9, 6}, mockGridPtr);

  ASSERT_EQ(object->getDescription(), "object@[9, 6]");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getPlayerId) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  object->init(2, {5, 5}, mockGridPtr);

  ASSERT_EQ(object->getPlayerId(), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, canPerformAction) {
  auto object = std::shared_ptr<Object>(new Object("object", 0, {}));

  object->addActionSrcBehaviour("can_perform", "ignored", "nop", {}, {});
  object->addActionDstBehaviour("cannot_perform", "ignored", "nop", {}, {});

  ASSERT_TRUE(object->canPerformAction("can_perform"));
  ASSERT_FALSE(object->canPerformAction("cannot_perform"));
}

TEST(ObjectTest, getParams) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto object = std::shared_ptr<Object>(new Object("object", 0, {{"test_param", std::make_shared<int32_t>(20)}}));

  ASSERT_EQ(*object->getParamValue("test_param"), 20);

  object->init(2, {5, 6}, mockGridPtr);

  ASSERT_EQ(*object->getParamValue("_x"), 5);
  ASSERT_EQ(*object->getParamValue("_y"), 6);

  ASSERT_EQ(object->getParamValue("does_not_exist"), nullptr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, actionBoundToSrc) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

CommandTestResult dstCommandTest(std::string commandName, std::vector<std::string> params, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialParams) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, initialParams));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  dstObject->addActionDstBehaviour("action", srcObjectName, commandName, params, {});

  auto behaviourResult = dstObject->onActionDst(srcObject, mockActionPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));

  return {behaviourResult, srcObject, dstObject};
}

CommandTestResult srcCommandTest(std::string commandName, std::vector<std::string> params, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialParams) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, initialParams));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

  ASSERT_EQ(*result.srcObject->getParamValue("test_param"), 21);
}

TEST(ObjectTest, src_command_decr) {
  auto result = srcCommandTest("decr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.srcObject->getParamValue("test_param"), 19);
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

  ASSERT_EQ(*result.dstObject->getParamValue("test_param"), 21);
}

TEST(ObjectTest, dst_command_decr) {
  auto result = dstCommandTest("decr", {"test_param"}, {{"test_param", std::make_shared<int32_t>(20)}});
  auto behaviourResult = result.behaviourResult;
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 0);

  ASSERT_EQ(*result.dstObject->getParamValue("test_param"), 19);
}

TEST(ObjectTest, src_command_mov_action_dest) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

  auto srcObjectStartLocation = GridLocation(3, 3);

  auto actionDestination = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
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

  ASSERT_EQ(*srcObject->getParamValue("_x"), 4);
  ASSERT_EQ(*srcObject->getParamValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_mov_action_src) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

  ASSERT_EQ(*srcObject->getParamValue("_x"), 4);
  ASSERT_EQ(*srcObject->getParamValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_mov_action_params) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";

  std::unordered_map<std::string, std::shared_ptr<int32_t>> initialParams;
  initialParams.insert({"mov_x", std::make_shared<int32_t>(7)});
  initialParams.insert({"mov_y", std::make_shared<int32_t>(12)});

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, initialParams));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

  ASSERT_EQ(*srcObject->getParamValue("_x"), 7);
  ASSERT_EQ(*srcObject->getParamValue("_y"), 12);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_dest) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

  auto dstObjectStartLocation = GridLocation(3, 3);

  auto actionDestination = GridLocation(4, 3);
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(1)
      .WillOnce(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
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

  ASSERT_EQ(*dstObject->getParamValue("_x"), 4);
  ASSERT_EQ(*dstObject->getParamValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_src) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

  ASSERT_EQ(*dstObject->getParamValue("_x"), 4);
  ASSERT_EQ(*dstObject->getParamValue("_y"), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dst_command_mov_action_params) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";

  std::unordered_map<std::string, std::shared_ptr<int32_t>> initialParams;
  initialParams.insert({"mov_x", std::make_shared<int32_t>(7)});
  initialParams.insert({"mov_y", std::make_shared<int32_t>(12)});

  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, initialParams));

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

  ASSERT_EQ(*dstObject->getParamValue("_x"), 7);
  ASSERT_EQ(*dstObject->getParamValue("_y"), 12);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_cascade) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

  auto srcObjectLocation = GridLocation(0, 0);
  auto dstObjectLocation = GridLocation(1, 0);
  auto cascadedObjectLocation = GridLocation(2, 0);

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, srcObjectLocation, mockGridPtr);
  dstObject->init(0, dstObjectLocation, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, getObject(Eq(cascadedObjectLocation)))
      .Times(1)
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*mockGridPtr, getObject(Eq(dstObjectLocation)))
      .Times(1)
      .WillOnce(Return(dstObject));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(2)
      .WillRepeatedly(Return("action"));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .Times(1)
      .WillOnce(Return(dstObjectLocation));

  EXPECT_CALL(*mockActionPtr, getDirection())
      .Times(1)
      .WillOnce(Return(Direction::RIGHT));

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
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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

TEST(ObjectTest, src_command_eq) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {{"resource", std::make_shared<int32_t>(0)}}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  ASSERT_EQ(*srcObject->getParamValue("resource"), 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_lt) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {{"counter", std::make_shared<int32_t>(0)},{"small", std::make_shared<int32_t>(-100)}, {"big", std::make_shared<int32_t>(100)}}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  ASSERT_EQ(*srcObject->getParamValue("counter"), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, src_command_gt) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {{"counter", std::make_shared<int32_t>(0)},{"small", std::make_shared<int32_t>(-100)}, {"big", std::make_shared<int32_t>(100)}}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

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
  ASSERT_EQ(*srcObject->getParamValue("counter"), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

}  // namespace griddy