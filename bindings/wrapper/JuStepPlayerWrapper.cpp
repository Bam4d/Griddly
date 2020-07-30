#pragma once

#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"


namespace griddly {
class Ju_StepPlayerWrapper {
 public:
  Ju_StepPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess)
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

  std::tuple<int,bool> step(std::string actionName, std::vector<int32_t> actionArray) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isStarted()) {
      throw std::invalid_argument("Cannot send player commands when game has not been started. start_game() must be called first.");
    }

    auto action = buildAction(actionName, actionArray);
    ActionResult actionResult;
    if (action != nullptr) {
      spdlog::debug("Player {0} performing action {1}", player_->getName(), action->getDescription());
      actionResult = player_->performActions({action});
    } else {
      actionResult = player_->performActions({});
    }

    int totalRewards = 0;
    for (auto &r : actionResult.rewards) {
      totalRewards += r;
    }

    return std::make_tuple(totalRewards, actionResult.terminated);
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    auto actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
    auto playerAvatar = player_->getAvatar();

    auto inputMappings = actionInputsDefinition.inputMappings;

    if (playerAvatar != nullptr) {
      auto actionId = actionArray[0];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      auto mapping = inputMappings[actionId];
      auto vectorToDest = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;
      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
      action->init(playerAvatar, vectorToDest, orientationVector, actionInputsDefinition.relative);

      return action;
    } else {
      glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};

      auto actionId = actionArray[2];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      auto mapping = inputMappings[actionId];
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