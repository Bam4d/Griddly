#include <memory>
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
using ::testing::ReturnRefOfCopy;
using ::testing::UnorderedElementsAre;

namespace griddly {

const std::string NOP = "nop";
const std::string ACTION = "action";

std::shared_ptr<MockAction> setupAction(std::string actionName, uint32_t originatingPlayerId, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockActionPtr, getMetaData(_))
      .WillRepeatedly(Return(0));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockActionPtr, getMetaData(_))
      .WillRepeatedly(Return(0));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject, std::unordered_map<std::string, int32_t> metaData) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(metaData));

  for (const auto& metaDataIt : metaData) {
    EXPECT_CALL(*mockActionPtr, getMetaData(metaDataIt.first))
        .WillRepeatedly(Return(metaDataIt.second));
  }

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockActionPtr, getMetaData(_))
      .WillRepeatedly(Return(0));

  return mockActionPtr;
}

std::shared_ptr<MockAction> setupAction(std::string actionName, std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destObject, glm::ivec2 destLocation) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockActionPtr, getMetaData(_))
      .WillRepeatedly(Return(0));

  return mockActionPtr;
}

TEST(ObjectTest, getLocation) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto object = std::make_shared<Object>(Object("object", 'o', 0, 0, {}, nullptr, mockGridPtr));

  object->init({5, 5});

  ASSERT_EQ(object->getLocation(), glm::ivec2(5, 5));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getObjectName) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto object = std::make_shared<Object>(Object("object", 'o', 0, 0, {}, nullptr, mockGridPtr));

  ASSERT_EQ(object->getObjectName(), "object");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getDescription) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto object = std::make_shared<Object>(Object("object", 'o', 0, 0, {}, nullptr, mockGridPtr));

  object->init({9, 6});

  ASSERT_EQ(object->getDescription(), "object@[9, 6]");

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getPlayerId) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto object = std::make_shared<Object>(Object("object", 'o', 2, 0, {}, nullptr, mockGridPtr));

  object->init({5, 5});

  ASSERT_EQ(object->getPlayerId(), 2);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, getVariables) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto object = std::make_shared<Object>(Object("object", 'o', 2, 0, {{"test_param", _V(20)}}, nullptr, mockGridPtr));

  ASSERT_EQ(*object->getVariableValue("test_param"), 20);

  object->init({5, 6});

  ASSERT_EQ(*object->getVariableValue("_x"), 5);
  ASSERT_EQ(*object->getVariableValue("_y"), 6);

  ASSERT_EQ(object->getVariableValue("does_not_exist"), nullptr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
}

TEST(ObjectTest, actionBoundToSrc) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  srcObject->addActionSrcBehaviour(ACTION, 0, dstObjectName, NOP, {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr, {0});

  ASSERT_FALSE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, actionBoundToDst) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  dstObject->addActionDstBehaviour(ACTION, 0, srcObjectName, NOP, {}, {});

  auto dstResult = dstObject->onActionDst(mockActionPtr, {0});

  ASSERT_FALSE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// Test that if the destination object returns _empty/is nullptr then we still perform source action commands based on the dstObjectName
// This can be the case when destination objects are removed during a behaviour
TEST(ObjectTest, actionDestinationObjectDifferentFromOriginalObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, glm::ivec2{1, 1});

  srcObject->addActionSrcBehaviour(ACTION, 0, dstObjectName, NOP, {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr, {0});

  ASSERT_FALSE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is registered for dst object and action, but not performed on different dst object
TEST(ObjectTest, srcActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  srcObject->addActionSrcBehaviour(ACTION, 0, "not_dst_object", NOP, {}, {});

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr, {0});

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, srcActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  auto srcResult = srcObject->onActionSrc(dstObjectName, mockActionPtr, {0});

  ASSERT_TRUE(srcResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(ObjectTest, dstActionNoBehaviourForDstObject) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  dstObject->addActionDstBehaviour(ACTION, 0, "not_src_object", NOP, {}, {});

  auto dstResult = dstObject->onActionDst(mockActionPtr, {0});

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

// source command is not registered for action
TEST(ObjectTest, dstActionNoBehaviourForAction) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(ACTION, srcObject, dstObject);

  auto dstResult = dstObject->onActionDst(mockActionPtr, {0});

  ASSERT_TRUE(dstResult.abortAction);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

struct CommandTestResult {
  BehaviourResult behaviourResult;
  std::shared_ptr<Object> srcObject;
  std::shared_ptr<Object> dstObject;
};

std::shared_ptr<Object> setupObject(uint32_t playerId, std::string objectname, glm::ivec2 location, DiscreteOrientation orientation, std::unordered_map<std::string, std::shared_ptr<int32_t>> initialVariables, std::shared_ptr<MockGrid> mockGridPtr, std::shared_ptr<ObjectGenerator> objectGenerator) {
  auto object = std::make_shared<Object>(Object(objectname, 'o', playerId, 0, initialVariables, objectGenerator, mockGridPtr));

  if (mockGridPtr != nullptr) {
    object->init(location, orientation);

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

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, CommandArguments commandArgumentMap, CommandList conditionalCommands, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  switch (type) {
    case ActionBehaviourType::DESTINATION: {
      dstObjectPtr->addActionDstBehaviour(action->getActionName(), 0, srcObjectPtr->getObjectName(), commandName, commandArgumentMap, conditionalCommands);
      return dstObjectPtr->onActionDst(action, {0});
    }
    case ActionBehaviourType::SOURCE: {
      auto dstObjectName = dstObjectPtr == nullptr ? "_empty" : dstObjectPtr->getObjectName();
      srcObjectPtr->addActionSrcBehaviour(action->getActionName(), 0, dstObjectName, commandName, commandArgumentMap, conditionalCommands);
      return srcObjectPtr->onActionSrc(dstObjectName, action, {0});
    }
  }

  return {true};
}

BehaviourResult addCommandsAndExecute(ActionBehaviourType type, std::shared_ptr<MockAction> action, std::string commandName, CommandArguments commandArgumentMap, std::shared_ptr<Object> srcObjectPtr, std::shared_ptr<Object> dstObjectPtr) {
  return addCommandsAndExecute(type, action, commandName, commandArgumentMap, {}, srcObjectPtr, dstObjectPtr);
}

std::shared_ptr<MockGrid> mockGrid() {
  auto mockGridPtr = std::make_shared<MockGrid>();
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
         action->getDelay() == delay &&
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

MATCHER_P3(SingletonActionMetaDataMatcher, actionName, sourceObjectPtr, expectedMetaData, "") {
  auto action = arg[0];

  const auto& actionMetaData = action->getMetaData();
  if (arg.size() == 1 &&
      action->getActionName() == actionName &&
      action->getSourceObject().get() == sourceObjectPtr.get() &&
      actionMetaData.size() == expectedMetaData.size()) {
    for (const auto& metaDataIt : expectedMetaData) {
      if (actionMetaData.at(metaDataIt.first) != metaDataIt.second) {
        return false;
      }
    }

    return true;
  }

  return false;
}

TEST(ObjectTest, command_reward) {
  auto srcObjectPtr = setupObject(1, "srcObject", {});
  auto dstObjectPtr = setupObject(3, "dstObject", {});
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("10")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("-10")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 10}});
  verifyCommandResult(dstResult, false, {{3, -10}});

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_reward_variable) {
  auto srcObjectPtr = setupObject(1, "srcObject", {{"ten", _V(10)}});
  auto dstObjectPtr = setupObject(3, "dstObject", {{"minusten", _V(-10)}});
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "reward", {{"0", _Y("ten")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "reward", {{"0", _Y("minusten")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 10}});
  verifyCommandResult(dstResult, false, {{3, -10}});

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, command_reward_default_to_action_player_id) {
  auto srcObjectPtr = setupObject(0, "srcObject", {});
  auto dstObjectPtr = setupObject(0, "dstObject", {});
  auto mockActionPtr = setupAction(ACTION, 2, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

  auto mockActionPtr1 = setupAction("do_exec", 5, srcObjectPtr, dstObjectPtr);
  auto mockActionPtr2 = setupAction("do_exec", 4, srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 5, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{2, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 4, glm::ivec2{0, -1})))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{10, 6}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr1, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y(ACTION)}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr2, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y(2)}, {"Executor", _Y(ACTION)}}, srcObjectPtr, dstObjectPtr);

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

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
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

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

TEST(ObjectTest, command_exec_metadata) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           MetaData:
  //*             variable: meta.action_variable
  //*             variable_value: 100
  //*             object_value: health
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           MetaData:
  //*             variable: meta.action_variable
  //*             variable_value: 50
  //*             object_value: health
  //*
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(), {{"health", _V(100)}}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(), {{"health", _V(20)}}, mockGridPtr, mockObjectGenerator);
  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr, {{"action_variable", 5}});

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                       },
                       false,
                       false}}};

  srand(100);

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

  std::unordered_map<std::string, int32_t> expectedSrcMetaData{
      {"variable", 5},
      {"variable_value", 100},
      {"object_value", 100},
  };

  std::unordered_map<std::string, int32_t> expectedDstMetaData{
      {"variable", 5},
      {"variable_value", 50},
      {"object_value", 20},
  };

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionMetaDataMatcher("exec_action", srcObjectPtr, expectedSrcMetaData)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionMetaDataMatcher("exec_action", dstObjectPtr, expectedDstMetaData)))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  YAML::Node srcActionMetaData;
  srcActionMetaData["variable"] = _Y("meta.action_variable");
  srcActionMetaData["variable_value"] = _Y("100");
  srcActionMetaData["object_value"] = _Y("health");

  YAML::Node dstActionMetadata;
  dstActionMetadata["variable"] = _Y("meta.action_variable");
  dstActionMetadata["variable_value"] = _Y("50");
  dstActionMetadata["object_value"] = _Y("health");

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"MetaData", srcActionMetaData}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"MetaData", dstActionMetadata}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 3}});
  verifyCommandResult(dstResult, false, {{1, 3}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_resolve_delay) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: delay
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: delay
  //*
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(), {{"delay", _V(10)}}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(), {{"delay", _V(20)}}, mockGridPtr, mockObjectGenerator);
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
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, srcObjectPtr, glm::ivec2(-1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 20, dstObjectPtr, glm::ivec2(-1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y("1")}, {"Delay", _Y("delay")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y("1")}, {"Delay", _Y("delay")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 3}});
  verifyCommandResult(dstResult, false, {{1, 3}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_resolve_action_id) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: object_variable
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           ActionId: object_variable
  //*
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(3, 3), DiscreteOrientation(), {{"object_variable", _V(1)}}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(6, 6), DiscreteOrientation(), {{"object_variable", _V(2)}}, mockGridPtr, mockObjectGenerator);
  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);

  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{-1, 0}, {-1, 0}, "Left"}},
                           {2, {{1, 0}, {1, 0}, "Right"}},
                       },
                       false,
                       false}}};

  srand(100);

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(2)
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", srcObjectPtr, 1, glm::ivec2(-1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonActionVectorOriginatingPlayerMatcher("exec_action", dstObjectPtr, 1, glm::ivec2(1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y("object_variable")}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"ActionId", _Y("object_variable")}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {{1, 3}});
  verifyCommandResult(dstResult, false, {{1, 3}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_exec_search) {
  //* - Src:
  //*     Object: srcObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Delay: 20
  //*           Search:
  //*             MaxDepth: 100
  //*             TargetObjectName: search_object
  //*   Dst:
  //*     Object: dstObject
  //*     Commands:
  //*       - exec:
  //*           Action: exec_action
  //*           Search:
  //*             TargetLocation: [6, 7]
  //*
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(1, "dstObject", glm::ivec2(5, 6), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);

  EXPECT_CALL(*mockGridPtr, getHeight()).WillRepeatedly(Return(100));
  EXPECT_CALL(*mockGridPtr, getWidth()).WillRepeatedly(Return(100));

  auto searchObjectPtr = setupObject(1, "search_object", glm::ivec2(5, 0), DiscreteOrientation(), {}, mockGridPtr, mockObjectGenerator);
  std::map<uint32_t, std::shared_ptr<Object>> noObjects = {};
  std::map<uint32_t, std::shared_ptr<Object>> searchObjectList = {{0, searchObjectPtr}};

  ON_CALL(*mockGridPtr, getObjectsAt(_)).WillByDefault(ReturnRef(noObjects));
  EXPECT_CALL(*mockGridPtr, getObjectsAt(_)).WillRepeatedly(ReturnRef(noObjects));
  EXPECT_CALL(*mockGridPtr, getObjectsAt(Eq(glm::ivec2(5, 0)))).WillRepeatedly(ReturnRef(searchObjectList));

  auto mockActionPtr = setupAction("do_exec", srcObjectPtr, dstObjectPtr);
  std::unordered_map<std::string, ActionInputsDefinition> mockInputDefinitions{
      {"exec_action", {{
                           {1, {{1, 0}, {0, 0}, ""}},
                           {2, {{-1, 0}, {0, 0}, ""}},
                           {3, {{1, 1}, {0, 0}, ""}},
                           {4, {{0, -1}, {0, 0}, ""}},
                       },
                       false,
                       false}}};

  auto invokeCollisionDetector = [this, searchObjectPtr](std::unordered_set<std::string> objectNames, std::string actionName, std::shared_ptr<CollisionDetector> collisionDetector) -> void {
    ASSERT_EQ(objectNames, std::unordered_set<std::string>({"search_object"}));
    collisionDetector->upsert(searchObjectPtr);
  };

  EXPECT_CALL(*mockGridPtr, addCollisionDetector).WillOnce(Invoke(invokeCollisionDetector));

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions())
      .Times(4)
      .WillRepeatedly(ReturnRefOfCopy(mockInputDefinitions));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 10, srcObjectPtr, glm::ivec2(1, 0))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(0), SingletonDelayedActionVectorMatcher("exec_action", 0, dstObjectPtr, glm::ivec2(1, 1))))
      .Times(1)
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 3}}));

  YAML::Node searchNodeTargetObjectName;
  YAML::Node searchNodeTargetLocation;

  searchNodeTargetObjectName["TargetObjectName"] = "search_object";
  searchNodeTargetLocation["TargetLocation"].push_back(6);
  searchNodeTargetLocation["TargetLocation"].push_back(7);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Delay", _Y(10)}, {"Search", searchNodeTargetObjectName}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "exec", {{"Action", _Y("exec_action")}, {"Search", searchNodeTargetLocation}}, srcObjectPtr, dstObjectPtr);

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
  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(2, "dstObject", glm::ivec2(1, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto globalVariables = std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>{
      {"global_variable", {{0, _V(10)}}}};

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

  EXPECT_CALL(*mockGridPtr, getGlobalVariables)
      .WillRepeatedly(ReturnRef(globalVariables));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(1), Eq(mockGridPtr)))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(2), Eq(mockGridPtr)))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(srcObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(0, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr), Eq(DiscreteOrientation())))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, removeObject(Eq(dstObjectPtr)))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(1, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr), Eq(DiscreteOrientation())))
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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto dstObjectPtr = setupObject(2, "dstObject", glm::ivec2(1, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();
  auto mockGridPtr = mockGrid();
  auto srcObjectPtr = setupObject(1, "srcObject", glm::ivec2(0, 0), Direction(), {}, mockGridPtr, mockObjectGenerator);
  auto newObjectPtr = setupObject("newObject", {});

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, glm::ivec2(1, 0));

  EXPECT_CALL(*mockObjectGenerator, newInstance(Eq("newObject"), Eq(1), Eq(mockGridPtr)))
      .WillOnce(Return(newObjectPtr));

  EXPECT_CALL(*mockGridPtr, addObject(Eq(glm::ivec2(1, 0)), Eq(newObjectPtr), Eq(true), Eq(mockActionPtr), Eq(DiscreteOrientation())))
      .Times(1);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

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

  auto mockActionPtr = setupAction(ACTION, srcObjectPtr, dstObjectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "neq", {{"0", _Y("resource")}, {"1", _Y("0")}}, {{"incr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);
  auto dstResult = addCommandsAndExecute(ActionBehaviourType::DESTINATION, mockActionPtr, "neq", {{"0", _Y("resource")}, {"1", _Y("1")}}, {{"decr", {{"0", _Y("resource")}}}}, srcObjectPtr, dstObjectPtr);

  verifyCommandResult(srcResult, false, {});
  verifyCommandResult(dstResult, false, {});

  ASSERT_EQ(*srcObjectPtr->getVariableValue("resource"), 2);
  ASSERT_EQ(*dstObjectPtr->getVariableValue("resource"), 1);

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_if_eq) {
  auto ifConditions = R"(
        Conditions: 
          eq: [0,1]
        OnTrue:
          - reward: 0
        OnFalse:
          - reward: 1
        )";

  auto ifNode = YAML::Load(ifConditions);

  auto commandArguments = singleOrListNodeToCommandArguments(ifNode);

  auto mockGridPtr = mockGrid();
  auto objectPtr = setupObject("object", {}, mockGridPtr);

  auto mockActionPtr = setupAction(ACTION, objectPtr, objectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "if", commandArguments, {}, objectPtr, objectPtr);

  verifyCommandResult(srcResult, false, {{1, 1}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_if_and) {
  auto ifConditions = R"(
        Conditions: 
          and:
            - eq: [0,1]
            - eq: [1,1]
        OnTrue:
          - reward: 0
        OnFalse:
          - reward: 1
        )";

  auto ifNode = YAML::Load(ifConditions);

  auto commandArguments = singleOrListNodeToCommandArguments(ifNode);

  auto mockGridPtr = mockGrid();
  auto objectPtr = setupObject("object", {}, mockGridPtr);

  auto mockActionPtr = setupAction(ACTION, objectPtr, objectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "if", commandArguments, {}, objectPtr, objectPtr);

  verifyCommandResult(srcResult, false, {{1, 1}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_if_or) {
  auto ifConditions = R"(
        Conditions: 
          or:
            - eq: [0,1]
            - eq: [1,1]
        OnTrue:
          - reward: 1
        OnFalse:
          - reward: 0
        )";

  auto ifNode = YAML::Load(ifConditions);

  auto commandArguments = singleOrListNodeToCommandArguments(ifNode);

  auto mockGridPtr = mockGrid();
  auto objectPtr = setupObject("object", {}, mockGridPtr);

  auto mockActionPtr = setupAction(ACTION, objectPtr, objectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "if", commandArguments, {}, objectPtr, objectPtr);

  verifyCommandResult(srcResult, false, {{1, 1}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, command_if_nested) {
  auto ifConditions = R"(
        Conditions: 
          and:
            - or: 
              - eq: [0,1]
              - eq: [1,1]
            - eq: [5,5]
        OnTrue:
          - reward: 1
        OnFalse:
          - reward: 0
        )";

  auto ifNode = YAML::Load(ifConditions);

  auto commandArguments = singleOrListNodeToCommandArguments(ifNode);

  auto mockGridPtr = mockGrid();
  auto objectPtr = setupObject("object", {}, mockGridPtr);

  auto mockActionPtr = setupAction(ACTION, objectPtr, objectPtr);

  auto srcResult = addCommandsAndExecute(ActionBehaviourType::SOURCE, mockActionPtr, "if", commandArguments, {}, objectPtr, objectPtr);

  verifyCommandResult(srcResult, false, {{1, 1}});

  verifyMocks(mockActionPtr, mockGridPtr);
}

TEST(ObjectTest, isValidAction) {
  auto srcObjectName = "srcObject";
  std::string dstObjectName = "dstObject";
  std::string actionName = ACTION;
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  auto preconditions = R"(
- eq: [counter,5]
)";

  auto preconditionsNode = YAML::Load(preconditions);

  srcObject->addPrecondition(actionName, 0, dstObjectName, preconditionsNode);
  srcObject->addActionSrcBehaviour(actionName, 0, dstObjectName, NOP, {}, {});

  auto preconditionResult = srcObject->isValidAction(mockActionPtr);

  // preconditions should come back as true because the counter value is equal to 5
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);
}

TEST(ObjectTest, multipleBehaviourPreconditionsSameObject) {
  auto srcObjectName = "srcObject";
  std::string dstObjectName = "dstObject";
  std::string actionName = ACTION;
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  auto preconditions1 = R"(
- eq: [counter,4]
)";

  auto preconditionsNode1 = YAML::Load(preconditions1);

  auto preconditions2 = R"(
- eq: [counter,5]
)";

  auto preconditionsNode2 = YAML::Load(preconditions2);

  srcObject->addPrecondition(actionName, 0, dstObjectName, preconditionsNode1);
  srcObject->addPrecondition(actionName, 1, dstObjectName, preconditionsNode2);
  srcObject->addActionSrcBehaviour(actionName, 0, dstObjectName, NOP, {}, {});
  srcObject->addActionSrcBehaviour(actionName, 1, dstObjectName, NOP, {}, {});

  auto behaviourIdxs = srcObject->getValidBehaviourIdxs(mockActionPtr);

  // preconditions should come back as true because the counter value is equal to 5
  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_THAT(behaviourIdxs, UnorderedElementsAre(1));
}

TEST(ObjectTest, isValidActionNotDefinedForAction) {
  auto srcObjectName = "srcObject";
  std::string dstObjectName = "dstObject";
  auto actionName = ACTION;
  std::string differentActionName = "different_action";
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  auto preconditions = R"(
- eq: [counter,5]
    )";

  auto preconditionsNode = YAML::Load(preconditions);

  srcObject->addPrecondition(differentActionName, 0, dstObjectName, preconditionsNode);
  srcObject->addActionSrcBehaviour(actionName, 0, dstObjectName, NOP, {}, {});

  auto preconditionResult = srcObject->isValidAction(mockActionPtr);

  ASSERT_EQ(*srcObject->getVariableValue("counter"), 5);
  ASSERT_TRUE(preconditionResult);

  verifyMocks(mockActionPtr);
}

TEST(ObjectTest, isValidActionNotDefinedForDestination) {
  auto srcObjectName = "srcObject";
  auto dstObjectName = "dstObject";
  std::string diffDstObjectName = "different_destination_object";
  std::string actionName = ACTION;
  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr, std::weak_ptr<Grid>()));
  auto dstObject = std::make_shared<Object>(Object(dstObjectName, 'D', 0, 0, {}, nullptr, std::weak_ptr<Grid>()));

  auto mockActionPtr = setupAction(actionName, srcObject, dstObject);

  auto preconditions = R"(
- eq: [counter,5]
    )";

  auto preconditionsNode = YAML::Load(preconditions);

  srcObject->addPrecondition(actionName, 0, diffDstObjectName, preconditionsNode);
  srcObject->addActionSrcBehaviour(actionName, 0, dstObjectName, NOP, {}, {});

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
  auto actionName = ACTION;

  auto mockGridPtr = mockGrid();

  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(10));
  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(10));

  auto srcObject = std::make_shared<Object>(Object(srcObjectName, 'S', 0, 0, {{"counter", _V(5)}}, nullptr, mockGridPtr));

  auto dstObjectOutside = std::make_shared<Object>(Object(dstBoundaryObjectName, 'S', 0, 0, {}, nullptr, mockGridPtr));
  auto dstObjectInside = std::make_shared<Object>(Object(dstEmptyObjectName, 'D', 0, 0, {}, nullptr, mockGridPtr));

  srcObject->init({5, 4}, DiscreteOrientation());

  dstObjectOutside->init({-1, -1}, DiscreteOrientation());
  dstObjectInside->init({5, 5}, DiscreteOrientation());

  auto mockActionPtrOutside = setupAction(actionName, srcObject, dstObjectOutside);
  auto mockActionPtrValid = setupAction(actionName, srcObject, dstObjectInside);

  srcObject->addActionSrcBehaviour(actionName, 0, dstEmptyObjectName, NOP, {}, {});
  srcObject->addActionSrcBehaviour(actionName, 0, dstBoundaryObjectName, NOP, {}, {});

  auto preconditionResultOutside = srcObject->isValidAction(mockActionPtrOutside);
  auto preconditionResultValid = srcObject->isValidAction(mockActionPtrValid);

  ASSERT_TRUE(preconditionResultOutside);
  ASSERT_TRUE(preconditionResultValid);

  verifyMocks(mockActionPtrOutside);
  verifyMocks(mockActionPtrValid);
}

TEST(ObjectTest, getInitialActions) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();

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

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions()).WillRepeatedly(ReturnRefOfCopy(mockActionInputDefinitions));
  EXPECT_CALL(*mockGridPtr, getRandomGenerator()).WillRepeatedly(Return(std::mt19937()));

  auto object = std::make_shared<Object>(Object(objectName, 'S', 0, 0, {}, mockObjectGenerator, mockGridPtr));

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
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockObjectGenerator = std::make_shared<MockObjectGenerator>();

  std::string objectName = "objectName";
  std::string originatingActionName = "originatingAction";

  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(originatingActionName));

  EXPECT_CALL(*mockActionPtr, getSourceLocation())
      .WillRepeatedly(Return(glm::ivec2{3, 3}));

  EXPECT_CALL(*mockActionPtr, getDestinationLocation())
      .WillRepeatedly(Return(glm::ivec2{4, 4}));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(glm::ivec2{3, 1}));

  EXPECT_CALL(*mockActionPtr, getOrientationVector())
      .WillRepeatedly(Return(glm::ivec2{1, 5}));

  EXPECT_CALL(*mockActionPtr, getMetaData())
      .WillRepeatedly(ReturnRefOfCopy(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockActionPtr, getMetaData(_))
      .WillRepeatedly(Return(0));

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

  EXPECT_CALL(*mockObjectGenerator, getActionInputDefinitions()).WillRepeatedly(ReturnRefOfCopy(mockActionInputDefinitions));
  EXPECT_CALL(*mockGridPtr, getRandomGenerator()).WillRepeatedly(Return(std::mt19937()));

  auto object = std::make_shared<Object>(Object(objectName, 'S', 0, 0, {}, mockObjectGenerator, mockGridPtr));

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