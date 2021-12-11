#include <functional>
#include <iostream>
#include <memory>

#include "Griddly/Core/GDY/TerminationHandler.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "Mocks/Griddly/Core/Players/MockPlayer.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Eq;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace griddly {

TEST(TerminationHandlerTest, terminateOnPlayerScore) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());
  auto mockPlayer2Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto player1Score = std::make_shared<int32_t>(10);
  auto player2Score = std::make_shared<int32_t>(5);

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(player1Score));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(player2Score));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"_score", "10"};
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}

TEST(TerminationHandlerTest, terminateOnPlayerObjects0) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());
  auto mockPlayer2Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto player1Score = std::make_shared<int32_t>(0);
  auto player2Score = std::make_shared<int32_t>(0);

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};
  auto mockBaseCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{1, std::make_shared<int32_t>(3)}, {2, std::make_shared<int32_t>(0)}};

  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("base")))
      .Times(1)
      .WillOnce(Return(mockBaseCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(player1Score));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(player2Score));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  // Player with 0 bases will end the game and "lose"
  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"base:count", "0"};
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1), Pair(2, 1)));
}

TEST(TerminationHandlerTest, terminateOnGlobalVariable) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());
  auto mockPlayer2Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto player1Score = std::make_shared<int32_t>(0);
  auto player2Score = std::make_shared<int32_t>(0);

  auto globalVariablePtr = std::make_shared<int32_t>(20);

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables;
  globalVariables["variable_name"].insert({0, globalVariablePtr});

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(player1Score));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(player2Score));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"variable_name", "20"};
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::NONE), Pair(2, TerminationState::NONE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 0), Pair(2, 0)));
}

TEST(TerminationHandlerTest, terminateOnPlayerGlobalVariable) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());
  auto mockPlayer2Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto player1Score = std::make_shared<int32_t>(0);
  auto player2Score = std::make_shared<int32_t>(0);

  auto player0Variable = std::make_shared<int32_t>(0);
  auto player1Variable = std::make_shared<int32_t>(0);
  auto player2Variable = std::make_shared<int32_t>(10);

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables;
  globalVariables["variable_name"].insert({0, player0Variable});
  globalVariables["variable_name"].insert({1, player1Variable});
  globalVariables["variable_name"].insert({2, player2Variable});

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(player1Score));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(player2Score));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  // Player with variable_name == 20 will win and the other player will lose
  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  tcd.commandArguments = {"variable_name", "0"};
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}

TEST(TerminationHandlerTest, terminateOnMaxTicks) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());
  auto mockPlayer2Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto player1Score = std::make_shared<int32_t>(0);
  auto player2Score = std::make_shared<int32_t>(0);

  auto tickCounter = std::make_shared<int32_t>(100);

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(player1Score));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(player2Score));

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .Times(1)
      .WillOnce(Return(tickCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"_steps", "100"};
  tcd.state = TerminationState::NONE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::NONE), Pair(2, TerminationState::NONE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 0), Pair(2, 0)));
}

TEST(TerminationHandlerTest, singlePlayer_differentId_win) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{0, std::make_shared<int32_t>(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("environment_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"environment_objects:count", "0"};
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1)));
}

TEST(TerminationHandlerTest, singlePlayer_differentId_lose) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{0, std::make_shared<int32_t>(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("environment_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = -1;
  tcd.opposingReward = 1;
  tcd.commandArguments = {"environment_objects:count", "0"};
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1)));
}

TEST(TerminationHandlerTest, singlePlayer_sameId_lose) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{1, std::make_shared<int32_t>(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = -1;
  tcd.opposingReward = 1;
  tcd.commandArguments = {"player_objects:count", "0"};
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1)));
}

TEST(TerminationHandlerTest, singlePlayer_sameId_win) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockPlayer1Ptr = std::shared_ptr<MockPlayer>(new MockPlayer());

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{1, std::make_shared<int32_t>(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::shared_ptr<TerminationHandler>(new TerminationHandler(mockGridPtr, players));

  TerminationConditionDefinition tcd;
  tcd.commandName = "eq";
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.commandArguments = {"player_objects:count", "0"};
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1)));
}

}  // namespace griddly