#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

namespace py = pybind11;

namespace griddly {
class Py_StepPlayerWrapper {
 public:
  Py_StepPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess)
      : player_(std::shared_ptr<Player>(new Player(playerId, playerName, observer))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
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

    auto action = buildAction(actionName, actionVector);
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
  const std::shared_ptr<GameProcess> gameProcess_;

  glm::vec2 getActionVectorFromId(uint32_t actionId) {
    switch (actionId) {
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

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    
    auto playerAvatar = player_->getAvatar();
    spdlog::debug("Player {0} controlling object {1}.", player_->getName(), playerAvatar->getObjectName());
    auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));

    switch (gdyFactory_->getActionControlScheme()) {
      case ActionControlScheme::DIRECT_RELATIVE: {
        // action Id 2 is "forward" it is the only action that
        glm::ivec2 actionVector = (actionArray[0] == 2) ? glm::ivec2{0, 0} : glm::ivec2{1, 0};
        action->init(playerAvatar, playerAvatar, actionVector, true);
      }
      break;
      case ActionControlScheme::DIRECT_ABSOLUTE: {
        auto actionVector = getActionVectorFromId(actionArray[0]);
        auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
        action->init(playerAvatar, actionVector, false);
      }
      break;
      case ActionControlScheme::SELECTION_RELATIVE: {
        glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};
        auto actionVector = getActionVectorFromId(actionArray[2]);
        auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
        action->init(playerAvatar, actionVector, true);
      }
      break;
      case ActionControlScheme::SELECTION_ABSOLUTE: {
        glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};
        auto actionVector = getActionVectorFromId(actionArray[2]);
        auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
        action->init(playerAvatar, actionVector, false);
      }
      break;
    }

    return action;
  }
};

}  // namespace griddly