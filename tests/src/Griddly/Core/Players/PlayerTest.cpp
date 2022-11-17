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
  int playerId = 0;
  std::string playerName = "PlayerName";
  std::string observerName = "ObserverName";

  Player player(playerId, playerName, observerName, nullptr);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), playerName);
  ASSERT_EQ(player.getObserverName(), observerName);
}

TEST(PlayerTest, performActions) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction());
  auto mockGameProcessPtr = std::make_shared<MockGameProcess>();

  int playerId = 0;
  std::string name = "PlayerName";
  std::string observerName = "ObserverName";
  auto player = std::shared_ptr<Player>(new Player(playerId, name, observerName, mockGameProcessPtr));

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
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction());
  auto mockGameProcessPtr = std::make_shared<MockGameProcess>();

  int playerId = 0;
  std::string name = "PlayerName";
  std::string observerName = "ObserverName";
  auto player = std::shared_ptr<Player>(new Player(playerId, name, observerName, mockGameProcessPtr));

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

}  // namespace griddly
