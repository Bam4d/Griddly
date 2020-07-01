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

  py::tuple step(std::string actionName, std::vector<int32_t> actionArray) {

    if(actionArray[0] == 0) {
      return py::make_tuple(0, false);
    }

    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    auto action = buildAction(actionName, actionArray);
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

  glm::ivec2 getActionVectorFromId(uint32_t actionId) {
    switch (actionId) {
      case 0:
      default:
        return {0, 0};
      case 1:
        return {-1, 0};
      case 2:
        return {0, -1};
      case 3:
        return {1, 0};
      case 4:
        return {0, 1};
    }
  }

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    
    auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));

    switch (gdyFactory_->getActionControlScheme()) {
      case ActionControlScheme::DIRECT_RELATIVE: {
        auto playerAvatar = player_->getAvatar();

        // action Id 2 is "forward" it is the only action that
        auto actionId = actionArray[0];

        if (actionId == 2) {
          action->init(playerAvatar, {0, -1}, true);
        } else if (actionId == 4) {
          action->init(playerAvatar, {0, 1}, true);
        } else {
          glm::ivec2 actionVector = getActionVectorFromId(actionArray[0]);
          action->init(playerAvatar, playerAvatar, actionVector, true);
        }
      }
      break;
      case ActionControlScheme::DIRECT_ABSOLUTE: {
        auto playerAvatar = player_->getAvatar();
        auto actionVector = getActionVectorFromId(actionArray[0]);
        action->init(playerAvatar, actionVector, false);
      }
      break;
      case ActionControlScheme::SELECTION_RELATIVE: {
        glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};
        auto actionVector = getActionVectorFromId(actionArray[2]);
        action->init(sourceLocation, sourceLocation+actionVector);
      }
      break;
      case ActionControlScheme::SELECTION_ABSOLUTE: {
        glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};
        auto actionVector = getActionVectorFromId(actionArray[2]);
        action->init(sourceLocation, sourceLocation+actionVector);
      }
      break;
    }

    return action;
  }
};

}  // namespace griddly