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

  auto dstResult = dstObject->onActionSrc(dstObject, mockActionPtr);

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

struct CommandTestResult {
  BehaviourResult behaviourResult;
  std::shared_ptr<Object> srcObject;
  std::shared_ptr<Object> dstObject;
};

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

TEST(ObjectTest, src_command_cascade) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::shared_ptr<Object>(new Object(srcObjectName, 0, {}));
  auto dstObject = std::shared_ptr<Object>(new Object(dstObjectName, 0, {}));

  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  srcObject->init(1, {0, 0}, mockGridPtr);
  EXPECT_CALL(*mockGridPtr, performActions)
      .Times(1)
      .WillOnce(Return(std::vector<int>{10}));

  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  EXPECT_CALL(*mockActionPtr, getActionName())
      .Times(2)
      .WillOnce(Return("action"));

  srcObject->addActionSrcBehaviour("action", dstObjectName, "cascade", {"_dest"}, {});

  auto behaviourResult = srcObject->onActionSrc(dstObject, mockActionPtr);
  ASSERT_FALSE(behaviourResult.abortAction);
  ASSERT_EQ(behaviourResult.reward, 10);

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
}

TEST(ObjectTest, src_command_gt) {
}

TEST(ObjectTest, src_command_lt) {
}

}  // namespace griddy