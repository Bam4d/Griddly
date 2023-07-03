#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GameProcess.hpp"
#include "Griddly/Core/Observers/TensorObservationInterface.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "WrapperCommon.cpp"

namespace py = pybind11;

namespace griddly {
class Py_StepPlayerWrapper {
 public:
  Py_StepPlayerWrapper(int playerId, std::string playerName, std::string observerName, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess)
      : player_(std::make_shared<Player>(Player(playerId, playerName, observerName, gameProcess))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
  }

  ~Py_StepPlayerWrapper() {
    spdlog::trace("StepPlayerWrapper Destroyed");
  }

  std::shared_ptr<Player> unwrapped() {
    return player_;
  }

  py::object getObservationDescription() const {
    return wrapObservationDescription(player_->getObserver());
  }

  py::object observe() {
    return wrapObservation(player_->getObserver());
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
      auto pStr = (int32_t*)stepArrayInfo.ptr + a * actionStride;

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

    auto actionResult = player_->performActions(actions, updateTicks);
    auto info_and_truncated = buildInfo(actionResult);
    auto info = info_and_truncated[0];
    auto truncated = info_and_truncated[1];
    auto rewards = gameProcess_->getAccumulatedRewards(player_->getId());
    return py::make_tuple(rewards, actionResult.terminated, truncated, info);
  }

  py::tuple stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks) {
    if (gameProcess_ != nullptr && !gameProcess_->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    auto action = buildAction(actionName, actionArray);

    ActionResult actionResult;
    if (action != nullptr) {
      actionResult = player_->performActions({action}, updateTicks);
    } else {
      actionResult = player_->performActions({}, updateTicks);
    }

    auto info_and_truncated = buildInfo(actionResult);
    auto info = info_and_truncated[0];
    auto truncated = info_and_truncated[1];

    return py::make_tuple(actionResult.terminated, truncated, info);
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

  py::tuple buildInfo(ActionResult actionResult) {
    py::dict py_info;
    bool truncated = false;

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
            playerStatusString = "End";
            break;
          case TerminationState::TRUNCATED:
            truncated = true;
            playerStatusString = "Truncated";
            break;
        }

        if (playerStatusString.size() > 0) {
          py_playerResults[std::to_string(playerRes.first).c_str()] = playerStatusString;
        }
      }
      py_info["PlayerResults"] = py_playerResults;
    }

    return py::make_tuple(py_info, truncated);
  }

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    const auto& actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
    auto playerAvatar = player_->getAvatar();
    auto playerId = player_->getId();

    const auto& inputMappings = actionInputsDefinition.inputMappings;

    if (playerAvatar != nullptr) {
      auto actionId = actionArray[0];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      const auto& mapping = inputMappings.at(actionId);
      const auto& vectorToDest = mapping.vectorToDest;
      const auto& orientationVector = mapping.orientationVector;
      const auto& metaData = mapping.metaData;
      const auto& action = std::make_shared<Action>(Action(gameProcess_->getGrid(), actionName, playerId, 0, metaData));
      action->init(playerAvatar, vectorToDest, orientationVector, actionInputsDefinition.relative);

      return action;
    } else {
      glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};

      auto actionId = actionArray[2];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      const auto& mapping = inputMappings.at(actionId);
      const auto& vector = mapping.vectorToDest;
      const auto& orientationVector = mapping.orientationVector;
      const auto& metaData = mapping.metaData;
      glm::ivec2 destinationLocation = sourceLocation + vector;

      auto action = std::make_shared<Action>(Action(gameProcess_->getGrid(), actionName, playerId, 0, metaData));
      action->init(sourceLocation, destinationLocation);

      return action;
    }
  }
};

}  // namespace griddly