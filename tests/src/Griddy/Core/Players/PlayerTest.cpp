#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGameProcess.cpp"
#include "Mocks/Griddy/Core/Observers/MockObserver.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"

#include "Griddy/Core/Players/Player.hpp"

using ::testing::ByMove;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ElementsAre;

namespace griddy {

TEST(PlayerTest, getIdAndName) {
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  int playerId = 0;
  std::string name = "PlayerName";

  Player player(playerId, name, nullptr);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), name);
}

TEST(PlayerTest, performActions) {
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction(6, 5));
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());
  
  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::shared_ptr<Player>(new Player(playerId, name, nullptr));

  player->init(10,10, mockGameProcessPtr);

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGameProcessPtr, performActions(Eq(playerId), Eq(actionsList)))
      .Times(1)
      .WillOnce(Return(std::vector<int>{0, 1, 2, 3, 4}));

  auto rewards = player->performActions(actionsList);

  ASSERT_THAT(rewards, ElementsAre(0,1,2,3,4));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGameProcessPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

TEST(PlayerTest, observe) {
  auto mockGrid = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGrid));
  auto mockObservationBytesPtr = std::unique_ptr<uint8_t[]>(new uint8_t[10 * 10]{});
  auto mockObservationBytes = mockObservationBytesPtr.get();

  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::shared_ptr<Player>(new Player(playerId, name, mockObserverPtr));

  EXPECT_CALL(*mockObserverPtr, update(Eq(playerId)))
      .Times(1)
      .WillOnce(Return(ByMove(std::move(mockObservationBytesPtr))));

  auto observation = player->observe();

  ASSERT_EQ(observation.get(), mockObservationBytes);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
}

}  // namespace griddy