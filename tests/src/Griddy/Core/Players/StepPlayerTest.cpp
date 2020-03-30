#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGameProcess.cpp"
#include "Mocks/Griddy/Core/MockGrid.cpp"

#include "Griddy/Core/Players/StepPlayer.hpp"

using ::testing::ByMove;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddy {

TEST(StepPlayerTest, getIdAndName) {
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  int playerId = 0;
  std::string name = "PlayerName";

  StepPlayer player(playerId, name);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), name);
}

TEST(StepPlayerTest, stepReturnsObservationAndReward) {
  auto mockActionPtr = std::shared_ptr<Action>(new MockAction(6, 5));
  auto mockObservationBytes = std::unique_ptr<uint8_t[]>(new uint8_t[10 * 10]{});
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  auto mockObservationBytesPtr = mockObservationBytes.get();

  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::shared_ptr<StepPlayer>(new StepPlayer(playerId, name));

  player->setGameProcess(mockGameProcessPtr);

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGameProcessPtr, observe(Eq(playerId)))
      .Times(1)
      .WillOnce(Return(ByMove(std::move(mockObservationBytes))));

  EXPECT_CALL(*mockGameProcessPtr, performActions(Eq(playerId), Eq(actionsList)))
      .Times(1)
      .WillOnce(Return(std::vector<int>{0, 1, 2, 3, 4}));

  auto stepResponse = player->step(actionsList);

  ASSERT_EQ(stepResponse.observation.get(), mockObservationBytesPtr);
  ASSERT_EQ(stepResponse.reward, 10);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGameProcessPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

}  // namespace griddy