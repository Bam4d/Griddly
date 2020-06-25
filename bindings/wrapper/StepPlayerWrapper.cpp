#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Actions/RelativeAction.hpp"
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

    spdlog::debug("Player {0} performing action {1}", player_->getName(), action->getDescription());
    auto action = buildAction(actionName, actionVector);
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

  glm::vec2 getActionVectorFromId(uint32_t actionId) {
    switch (actionId_) {
      case 0:
      default:
        return {0, 0};
      case 1:
        return {1, 0};
      case 2:
        return {0, 1};
      case 3:
        return {-1, 0};
      case 4:
        return {0, -1};
    }
  }

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionVector) {
    switch (gdyFactory_->getActionControlScheme()) {
      case ActionControlScheme::DIRECT_RELATIVE: {
        auto playerAvatar = player_->getAvatar();
        spdlog::debug("Player {0} controlling object {1}.", player_->getName(), playerAvatar->getObjectName());
        // action Id 2 is "forward" it is the only action that
        glm::ivec2 actionVector = actionVector[0] == 2 ? {0, 0} : {1, 0};
        auto action = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), actionName, 0));
        action.init(playerAvatar, playerAvatar, actionVector, true);
        return action;
      }
      case ActionControlScheme::DIRECT_ABSOLUTE: {
        auto playerAvatar = player_->getAvatar();
        spdlog::debug("Player {0} controlling object {1}.", player_->getName(), playerAvatar->getObjectName());

        auto actionVector = getActionVectorFromId(actionVector[0]);

        auto action = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), actionName, 0));
        action.init(playerAvatar, actionVector, false);
        return action;
      }
      case ActionControlScheme::SELECTION_RELATIVE: {
        glm::ivec2 sourceLocation = {actionVector[0], actionVector[1]};

        auto actionVector = getActionVectorFromId(actionVector[2]);
        auto action = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), actionName, 0));
        action.init(playerAvatar, actionVector, true);
      }
      case ActionControlScheme::SELECTION_ABSOLUTE: {
        glm::ivec2 sourceLocation = {actionVector[0], actionVector[1]};

        auto actionVector = getActionVectorFromId(actionVector[2]);
        auto action = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), actionName, 0));
        action.init(playerAvatar, actionVector, false);
      }
    }
  }
};

}  // namespace griddly