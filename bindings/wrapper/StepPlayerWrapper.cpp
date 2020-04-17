#pragma once
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddy/Core/Actions/ActionTypes.hpp"
#include "../../src/Griddy/Core/Actions/Gather.hpp"
#include "../../src/Griddy/Core/Actions/Move.hpp"
#include "../../src/Griddy/Core/Actions/Punch.hpp"
#include "../../src/Griddy/Core/Players/Player.hpp"

namespace griddy {
class Py_StepPlayerWrapper {
 public:
  Py_StepPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<Observer> observer) : player_(std::shared_ptr<Player>(new Player(playerId, playerName, observer))) {
  }

  std::shared_ptr<Player> unwrapped() {
    return player_;
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {

    auto observer = player_->getObserver();
    if (observer == nullptr) {
      throw std::invalid_argument("No player observer configured");
    }

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), player_->observe()));
  }

  int step(uint x, uint y, ActionType actionType, Direction direction) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    std::shared_ptr<Action> action;
    switch (actionType) {
      case MOVE:
        action = std::shared_ptr<Action>(new Move(direction, {x, y}));
        break;
      case GATHER:
        action = std::shared_ptr<Action>(new Gather(direction, {x, y}));
        break;
      case PUNCH:
        action = std::shared_ptr<Action>(new Punch(direction, {x, y}));
        break;
    }

    spdlog::debug("Player {0} performing action {1}", player_->getName(), action->getDescription());

    auto rewards = player_->performActions({action});

    int totalRewards = 0;
    for (auto &r : rewards) {
      totalRewards += r;
    }

    return totalRewards;
  }

 private:
  const std::shared_ptr<Player> player_;
};
}  // namespace griddy