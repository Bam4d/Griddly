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

  std::array<uint32_t, 2> getTileSize() const {
    auto tileSize = player_->getObserver()->getTileSize();
    return {(uint32_t)tileSize[0], (uint32_t)tileSize[1]};
  }

  std::vector<uint32_t> getObservationShape() const {
    return player_->getObserver()->getShape();
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {
    auto observer = player_->getObserver();
    if (observer == nullptr) {
      throw std::invalid_argument("No player observer configured");
    }

    auto observationData = observer->update();

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), observationData));
  }

  py::tuple stepMulti(py::buffer stepArray, bool updateTicks) {
    auto externalActionNames = gdyFactory_->getExternalActionNames();
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    auto stepArrayInfo = stepArray.request();
    if (stepArrayInfo.format != "l" && stepArrayInfo.format != "i") {
      auto error = fmt::format("Invalid data type {0}, must be an integer.", stepArrayInfo.format);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto actionStride = stepArrayInfo.strides[0] / sizeof(int32_t);
    auto actionArrayStride = stepArrayInfo.strides[1] / sizeof(int32_t);

    auto actionCount = stepArrayInfo.shape[0];
    auto actionSize = stepArrayInfo.shape[1];

    spdlog::debug("action stride: {0}", actionStride);
    spdlog::debug("action array stride: {0}", actionArrayStride);
    spdlog::debug("action count: {0}", actionCount);
    spdlog::debug("action size: {0}", actionSize);

    std::vector<std::shared_ptr<Action>> actions;
    for (int a = 0; a < actionCount; a++) {
      std::string actionName;
      std::vector<int32_t> actionArray;
      auto pStr = (int32_t *)stepArrayInfo.ptr + a * actionStride;

      switch (actionSize) {
        case 1:
          actionName = externalActionNames.at(0);
          actionArray.push_back(*(pStr + 0 * actionArrayStride));
          break;
        case 2:
          actionName = externalActionNames.at(*(pStr + 0 * actionArrayStride));
          actionArray.push_back(*(pStr + 1 * actionArrayStride));
          break;
        case 3:
          actionArray.push_back(*(pStr + 0 * actionArrayStride));
          actionArray.push_back(*(pStr + 1 * actionArrayStride));
          actionName = externalActionNames.at(0);
          actionArray.push_back(*(pStr + 2 * actionArrayStride));
          break;
        case 4:
          actionArray.push_back(*(pStr + 0 * actionArrayStride));
          actionArray.push_back(*(pStr + 1 * actionArrayStride));
          actionName = externalActionNames.at(*(pStr + 2 * actionArrayStride));
          actionArray.push_back(*(pStr + 3 * actionArrayStride));
          break;
        default: {
          auto error = fmt::format("Invalid action size, {0}", actionSize);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }
      }

      auto action = buildAction(actionName, actionArray);
      if (action != nullptr) {
        actions.push_back(action);
      }
    }

    return performActions(actions, updateTicks);
  }

  py::tuple stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    auto action = buildAction(actionName, actionArray);

    if (action != nullptr) {
      return performActions({action}, updateTicks);
    } else {
      return performActions({}, updateTicks);
    }
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

  py::tuple performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
    auto actionResult = player_->performActions(actions, updateTicks);
    auto info = buildInfo(actionResult);
    return py::make_tuple(actionResult.reward, actionResult.terminated, info);
  }

  py::dict buildInfo(ActionResult actionResult) {
    py::dict py_info;

    if (actionResult.terminated) {
      py::dict py_playerResults;

      for (auto playerRes : actionResult.playerStates) {
        std::string playerStatusString;
        switch (playerRes.second) {
          case TerminationState::WIN:
            playerStatusString = "Win";
            break;
          case TerminationState::LOSE:
            playerStatusString = "Lose";
            break;
          case TerminationState::NONE:
            playerStatusString = "";
            break;
        }

        if (playerStatusString.size() > 0) {
          py_playerResults[std::to_string(playerRes.first).c_str()] = playerStatusString;
        }
      }
      py_info["PlayerResults"] = py_playerResults;
    }

    return py_info;
  }

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    auto actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
    auto playerAvatar = player_->getAvatar();
    auto playerId = player_->getId();

    auto inputMappings = actionInputsDefinition.inputMappings;

    if (playerAvatar != nullptr) {
      auto actionId = actionArray[0];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      auto mapping = inputMappings[actionId];
      auto vectorToDest = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;
      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, playerId, 0));
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

      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, playerId, 0));
      action->init(sourceLocation, destinationLocation);

      return action;
    }
  }
};

}  // namespace griddly