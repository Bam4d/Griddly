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
    if (actionArray[0] == 0) {
      return py::make_tuple(0, false);
    }

    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    auto action = buildAction(actionName, actionArray);
    if(action != nullptr) {
      spdlog::debug("Player {0} performing action {1}", player_->getName(), action->getDescription());

      auto actionResult = player_->performActions({action});

      int totalRewards = 0;
      for (auto &r : actionResult.rewards) {
        totalRewards += r;
      }

      return py::make_tuple(totalRewards, actionResult.terminated);
    } else {
      return py::make_tuple(0, false);
    }
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {

    auto actionMapping = gdyFactory_->findActionMapping(actionName);

    auto playerAvatar = player_->getAvatar();
    if (playerAvatar != nullptr) {
      auto actionId = actionArray[0];

      if(actionMapping.inputMap.find(actionId) == actionMapping.inputMap.end()) {
        return nullptr;
      }
      
      auto mapping = actionMapping.inputMap[actionId];
      auto vectorToDest = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;
      
      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
      action->init(playerAvatar, vectorToDest, orientationVector, actionMapping.relative);

      return action;
    } else {
      glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};

      auto actionId = actionArray[2];

      if(actionMapping.inputMap.find(actionId) == actionMapping.inputMap.end()) {
        return nullptr;
      }

      auto mapping = actionMapping.inputMap[actionId];
      auto vector = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;

      glm::ivec2 destinationLocation = sourceLocation + vector;

      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
      action->init(sourceLocation, destinationLocation);

      return action;
    }

    
  }
};

}  // namespace griddly