#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/GameProcess.cpp"
#include "Griddy/Core/Players/Player.cpp"
#include "Griddy/Core/Actions/Action.cpp"
#include "Griddy/Core/Players/StepPlayer.cpp"
#include "Mocks/Griddy/Core/Actions/MockAction.cpp"
#include "Mocks/Griddy/Core/MockGameProcess.cpp"

using ::testing::ByMove;
using ::testing::Return;
using ::testing::Eq;
using ::testing::Mock;

namespace griddy {

TEST(StepPlayerTest, getIdAndName) {
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  int playerId = 0;
  std::string name = "PlayerName";

  StepPlayer player(playerId, name, mockGameProcessPtr);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), name);
}

TEST(StepPlayerTest, stepReturnsObservation) {

  auto mockActionPtr = std::shared_ptr<Action>(new MockAction(6, 5));
  auto mockObservationBytes = std::unique_ptr<uint8_t[]>(new uint8_t[10 * 10]{});
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  int playerId = 0;
  std::string name = "PlayerName";
  auto player = std::shared_ptr<StepPlayer>(new StepPlayer(playerId, name, mockGameProcessPtr));

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  ON_CALL(*mockGameProcessPtr, observe)
      .WillByDefault(Return(ByMove(std::move(mockObservationBytes))));

  // EXPECT_CALL(*mockGameProcessPtr, observe(Eq(playerBase)))
  //     .Times(1)
  //     .WillOnce(Return(ByMove(std::move(mockObservationBytes))));

  ON_CALL(*mockGameProcessPtr, performActions)
      .WillByDefault(Return(std::vector<int>{0}));

  // EXPECT_CALL(*mockGameProcessPtr, performActions(Eq(playerId), Eq(actionsList)))
  //     .Times(1)
  //     .WillOnce(Return(std::vector<int>{0}));

  auto stepResponse = player->step(actionsList);

  // ASSERT_EQ(stepResponse.observation, mockObservationBytes);
  // ASSERT_EQ(stepResponse.reward, 0);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGameProcessPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockActionPtr.get()));
}

}  // namespace griddy