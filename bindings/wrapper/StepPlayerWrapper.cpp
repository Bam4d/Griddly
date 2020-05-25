#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Objects/GridLocation.hpp"
#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

namespace py = pybind11;

namespace griddly {
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

  Direction getDirection(uint32_t directionIdx) {
    switch (directionIdx) {
      case 0:
        return Direction::NONE;
      case 1:
        return Direction::LEFT;
      case 2:
        return Direction::DOWN;
      case 3:
        return Direction::RIGHT;
      case 4:
        return Direction::UP;
    }
  }

  py::tuple step(std::string actionName, std::vector<uint32_t> actionVector) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    auto playerAvatar = player_->getAvatar();
    uint32_t actionX = 0;
    uint32_t actionY = 0;

    Direction direction;
    if (playerAvatar != nullptr) {
      auto sourceLocation = playerAvatar->getLocation();
      actionX = sourceLocation.x;
      actionY = sourceLocation.y;
      spdlog::debug("Player {0} controlling object {1} at location [{2}, {3}]", player_->getName(), playerAvatar->getObjectName(), actionX, actionY);
      direction = getDirection(actionVector[0]);
    } else {
      actionX = actionVector[0];
      actionY = actionVector[1];
      direction = getDirection(actionVector[2]);
    }

    auto action = std::shared_ptr<Action>(new Action(actionName, {actionX, actionY}, direction));

    spdlog::debug("Player {0} performing action {1}", player_->getName(), action->getDescription());

    auto actionResult = player_->performActions({action});

    int totalRewards = 0;
    for (auto &r : actionResult.rewards) {
      totalRewards += r;
    }

    

    return py::make_tuple(totalRewards, actionResult.terminated);
  }

 private:
  const std::shared_ptr<Player> player_;
};
}  // namespace griddly