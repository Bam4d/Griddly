#include "gtest/gtest.h"

#include "Griddy/Core/Player/StepPlayer.cpp"
#include "Mocks/Griddy/Core/MockGameProcess.cpp"

namespace griddy {

TEST(StepPlayerTest, getHeightAndWidth) {
  auto mockGameProcess = std::shared_ptr<MockGameProcess>(new MockGameProcess());

  int playerId = 0;
  std::string name = "PlayerName";

  StepPlayer player(playerId, name, mockGameProcess);

  ASSERT_EQ(player.getId(), playerId);
  ASSERT_EQ(player.getName(), name);
}

}