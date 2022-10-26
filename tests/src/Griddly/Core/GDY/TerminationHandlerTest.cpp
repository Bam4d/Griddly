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

#define _V(X) std::make_shared<int32_t>(X)

namespace griddly {

TEST(TerminationHandlerTest, terminateOnPlayerScore) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(10)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(5)));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ _score, 10 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}

TEST(TerminationHandlerTest, terminateOnPlayerScore_list) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(10)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(5)));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("[ eq: [ _score, 10 ], eq: [ _score, 10 ] ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}

TEST(TerminationHandlerTest, terminateOnPlayerScore_singleton_list) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(10)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(5)));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("- eq: [ _score, 10 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}


TEST(TerminationHandlerTest, terminateOnPlayerObjects0) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};
  auto mockBaseCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{1, _V(3)}, {2, _V(0)}};

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
      .WillRepeatedly(Return(_V(0)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ base:count, 0 ]");

  // Player with 0 bases will end the game and "lose"
  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1), Pair(2, 1)));
}

TEST(TerminationHandlerTest, terminateOnGlobalVariable) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables;
  globalVariables["variable_name"].insert({0, _V(20)});

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ variable_name, 20 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::NONE), Pair(2, TerminationState::NONE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 0), Pair(2, 0)));
}

TEST(TerminationHandlerTest, terminateOnPlayerGlobalVariable) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables;
  globalVariables["variable_name"].insert({0, _V(0)});
  globalVariables["variable_name"].insert({1, _V(0)});
  globalVariables["variable_name"].insert({2, _V(10)});

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ variable_name, 0 ]");

  // Player with variable_name == 20 will win and the other player will lose
  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, -1)));
}

TEST(TerminationHandlerTest, terminateOnMaxTicks) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();
  auto mockPlayer2Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr, mockPlayer2Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockPlayer2Ptr, getId())
      .WillRepeatedly(Return(2));

  EXPECT_CALL(*mockPlayer1Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  EXPECT_CALL(*mockPlayer2Ptr, getScore())
      .WillRepeatedly(Return(_V(0)));

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .Times(1)
      .WillOnce(Return(_V(100)));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ _steps, 100 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::NONE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::NONE), Pair(2, TerminationState::NONE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 0), Pair(2, 0)));
}

TEST(TerminationHandlerTest, singlePlayer_objectCounter_lose) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{0, _V(0)}, {1, _V(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ player_objects:count, 0 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = -1;
  tcd.opposingReward = 1;
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1)));
}

TEST(TerminationHandlerTest, singlePlayer_objectCounter_win) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{{0, _V(0)}, {1, _V(0)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ player_objects:count, 0 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1)));
}

TEST(TerminationHandlerTest, multiPlayer_objectCounter_lose) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{
      {0, _V(0)},
      {1, _V(0)},
      {2, _V(5)},
      {3, _V(5)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ player_objects:count, 0 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = -1;
  tcd.opposingReward = 1;
  tcd.state = TerminationState::LOSE;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::LOSE), Pair(2, TerminationState::WIN), Pair(3, TerminationState::WIN)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1), Pair(2, 1), Pair(3, 1)));
}

TEST(TerminationHandlerTest, multiPlayer_objectCounter_multipleWinners) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{
      {0, _V(0)},
      {1, _V(0)},
      {2, _V(0)},
      {3, _V(5)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load("eq: [ player_objects:count, 0 ]");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::WIN), Pair(2, TerminationState::WIN), Pair(3, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, 1), Pair(2, 1), Pair(3, -1)));
}

TEST(TerminationHandlerTest, multiPlayer_objectCounter_multipleConditions) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockPlayer1Ptr = std::make_shared<MockPlayer>();

  auto players = std::vector<std::shared_ptr<Player>>{mockPlayer1Ptr};

  EXPECT_CALL(*mockPlayer1Ptr, getId())
      .WillRepeatedly(Return(1));

  auto playerObjectCounter = std::unordered_map<uint32_t, std::shared_ptr<int32_t>>{
      {0, _V(0)},
      {1, _V(0)},
      {2, _V(0)},
      {3, _V(5)}};
  EXPECT_CALL(*mockGridPtr, getObjectCounter(Eq("player_objects")))
      .Times(1)
      .WillOnce(Return(playerObjectCounter));

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{
      {"player_var", {{0, _V(0)}, {1, _V(0)}, {2, _V(2)}, {3, _V(5)}}}};
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .Times(1)
      .WillOnce(ReturnRef(globalVariables));

  auto terminationHandlerPtr = std::make_shared<TerminationHandler>(mockGridPtr, players);

  auto conditionsNode = YAML::Load(R"(
and: 
  - eq: [ player_objects:count, 0 ]
  - gt: [ player_var, 1 ]
)");

  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.reward = 1;
  tcd.opposingReward = -1;
  tcd.state = TerminationState::WIN;
  terminationHandlerPtr->addTerminationCondition(tcd);

  auto terminationResult = terminationHandlerPtr->isTerminated();

  ASSERT_TRUE(terminationResult.terminated);
  ASSERT_THAT(terminationResult.playerStates, UnorderedElementsAre(Pair(1, TerminationState::LOSE), Pair(2, TerminationState::WIN), Pair(3, TerminationState::LOSE)));
  ASSERT_THAT(terminationResult.rewards, UnorderedElementsAre(Pair(1, -1), Pair(2, 1), Pair(3, -1)));
}

}  // namespace griddly