#include "StepPlayer.hpp"

namespace griddy {

StepPlayer::StepPlayer(int id, std::string playerName)
    : Player(id, playerName) {
}

StepPlayer::~StepPlayer() {
}

StepObservation StepPlayer::step(std::vector<std::shared_ptr<Action>> actions) {
  auto reward = Player::performActions(actions);

  auto observation = Player::observe();

  int rewardSum = 0;
  for (auto& n : reward) {
    rewardSum += n;
  }

  return {std::move(observation), rewardSum, {}};
}
}  // namespace griddy