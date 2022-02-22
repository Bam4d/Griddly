#include <memory>

#include "Griddly/Core/Players/Player.hpp"
#include "Mocks/Griddly/Core/GDY/Actions/MockAction.hpp"
#include "Mocks/Griddly/Core/MockGameProcess.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "Mocks/Griddly/Core/Observers/MockObserver.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

TEST(PlayerTest, getIdAndName) {
  auto mockGameProcessPtr = std::make_shared<MockGameProcess>();

  int playerId = 0;
  std::string name = "PlayerName";

  Player player(playerId, name, nullptr);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), name);
}

TEST(PlayerTest, performActions) {
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction());
  auto mockGameProcessPtr = std::make_shared<MockGameProcess>();

  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::make_shared<Player>(playerId, name, nullptr);

  ObserverConfig observerConfig{};

  player->init(observerConfig, false, mockGameProcessPtr);

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGameProcessPtr, performActions(Eq(playerId), Eq(actionsList), Eq(true)))
      .Times(1)
      .WillOnce(Return(ActionResult{{}, false}));

  auto actionResult = player->performActions(actionsList);
  auto terminated = actionResult.terminated;

  EXPECT_FALSE(terminated);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGameProcessPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(PlayerTest, performActions_terminated) {
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction());
  auto mockGameProcessPtr = std::make_shared<MockGameProcess>();

  int playerId = 1;
  std::string name = "PlayerName";
  auto player = std::make_shared<Player>(playerId, name, nullptr);

  ObserverConfig observerConfig{};

  player->init(observerConfig, false, mockGameProcessPtr);

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGameProcessPtr, performActions(Eq(playerId), Eq(actionsList), Eq(true)))
      .Times(1)
      .WillOnce(Return(ActionResult{{{1, TerminationState::WIN}}, true}));

  auto actionResult = player->performActions(actionsList);
  auto terminated = actionResult.terminated;
  auto states = actionResult.playerStates;

  EXPECT_EQ(states, (std::unordered_map<uint32_t, TerminationState>{{1, TerminationState::WIN}}));
  EXPECT_TRUE(terminated);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGameProcessPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(PlayerTest, observe) {
  auto mockGrid = std::make_shared<MockGrid>();
  auto mockObserverPtr = std::make_shared<MockObserver>(mockGrid);
  auto mockObservationBytesPtr = new uint8_t[10 * 10]{};

  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::make_shared<Player>(playerId, name, mockObserverPtr);

  EXPECT_CALL(*mockObserverPtr, update())
      .Times(1)
      .WillOnce(Return(mockObservationBytesPtr));

  auto observation = player->observe();

  ASSERT_EQ(observation, mockObservationBytesPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
}

}  // namespace griddly