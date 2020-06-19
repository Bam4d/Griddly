#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Actions/RelativeAction.hpp"
#include "../../src/Griddly/Core/GDY/Objects/GridLocation.hpp"
#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

namespace py = pybind11;

namespace griddly {
class Py_StepPlayerWrapper {
 public:
  Py_StepPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory)
      : player_(std::shared_ptr<Player>(new Player(playerId, playerName, observer))), gdyFactory_(gdyFactory) {
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

  py::tuple step(std::string actionName, std::vector<int32_t> actionVector) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    auto playerAvatar = player_->getAvatar();
    int32_t actionX = 0;
    int32_t actionY = 0;

    uint32_t actionId;
    if (playerAvatar != nullptr) {
      auto sourceLocation = playerAvatar->getLocation();
      actionX = sourceLocation.x;
      actionY = sourceLocation.y;
      spdlog::debug("Player {0} controlling object {1} at location [{2}, {3}]", player_->getName(), playerAvatar->getObjectName(), actionX, actionY);
      actionId = actionVector[0];
    } else {
      actionX = actionVector[0];
      actionY = actionVector[1];
      actionId = actionVector[2];
    }

    auto actionMapping = gdyFactory_->getActionControlScheme();

    std::shared_ptr<Action> action;
    switch (actionMapping) {
      case ActionControlScheme::DIRECT_RELATIVE:
        action = std::shared_ptr<Action>(new RelativeAction(actionName, {actionX, actionY}, actionId));
        break;
      case ActionControlScheme::DIRECT_ABSOLUTE:
        action = std::shared_ptr<Action>(new Action(actionName, {actionX, actionY}, actionId));
        break;
      case ActionControlScheme::SELECTION_ABSOLUTE:
        action = std::shared_ptr<Action>(new Action(actionName, {actionX, actionY}, actionId));
        break;
      case ActionControlScheme::SELECTION_RELATIVE:
        action = std::shared_ptr<Action>(new RelativeAction(actionName, {actionX, actionY}, actionId));
        break;
    }

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
  const std::shared_ptr<GDYFactory> gdyFactory_;
};
}  // namespace griddly