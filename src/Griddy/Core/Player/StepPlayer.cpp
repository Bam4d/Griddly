#include "StepPlayer.hpp"

namespace griddy {

StepPlayer::StepPlayer(int id, std::string playerName, std::shared_ptr<GameProcess> gameProcess)
    : Player(id, playerName, gameProcess) {
}

StepObservation StepPlayer::step(std::vector<std::shared_ptr<Action>> actions) {
  auto reward = Player::act(actions);

  auto observation = Player::observe();

  return {std::move(observation), reward, {}};
}
}  // namespace griddy