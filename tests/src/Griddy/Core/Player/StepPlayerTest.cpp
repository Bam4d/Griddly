#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/GameProcess.cpp"
#include "Griddy/Core/Player/Player.cpp"
#include "Griddy/Core/Player/StepPlayer.cpp"
#include "Mocks/Griddy/Core/MockGameProcess.cpp"

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
  auto mockActionPtr = std::shared_ptr<MockAction>(new MockAction());
  auto mockObservationBytes = std::unique_ptr<uint8_t[]>(new uint8_t[10 * 10]{})
  auto mockGameProcessPtr = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  EXPECT_CALL(*mockGameProcess, observe())
      .Times(1)
      .WillByDefault(Return(mockObservationBytes));

  EXPECT_CALL(*mockGameProcess, performAction({mockActionPtr}))
      .Times(1)
      .WillByDefault(Return(mockObservationBytes));

  int playerId = 0;
  std::string name = "PlayerName";

  StepPlayer player(playerId, name, mockGameProcess);

  auto stepResponse = player.step();
}

}  // namespace griddy