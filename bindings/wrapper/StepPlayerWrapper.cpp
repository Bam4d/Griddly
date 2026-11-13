#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectRules.hpp"
#include "Griddly/Core/GameProcess.hpp"
#include "Griddly/Core/Observers/TensorObservationInterface.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "WrapperCommon.cpp"

namespace py = nanobind;

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
    RawObservation raw;
    {
      py::gil_scoped_release release;
      std::lock_guard<std::recursive_mutex> lock(*mutex_);
      raw = updateObservationRaw(player_->getObserver());
    }
    return wrapObservationRaw(raw);
  }

  py::tuple stepMulti(py::ndarray<int32_t, py::ndim<2>> stepArray, bool updateTicks) {
    auto externalActionNames = gdyFactory_->getExternalActionNames();
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    auto& stepArrayInfo = stepArray;
    if (stepArrayInfo.dtype() != py::dtype<int32_t>() && stepArrayInfo.dtype() != py::dtype<int64_t>()) {
      auto error = fmt::format("Invalid data type, must be an integer.");
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto actionStride = stepArrayInfo.stride(0) / sizeof(int32_t);
    auto actionArrayStride = stepArrayInfo.stride(1) / sizeof(int32_t);

    auto actionCount = stepArrayInfo.shape(0);
    auto actionSize = stepArrayInfo.shape(1);

    spdlog::debug("action stride: {0}", actionStride);
    spdlog::debug("action array stride: {0}", actionArrayStride);
    spdlog::debug("action count: {0}", actionCount);
    spdlog::debug("action size: {0}", actionSize);

    std::vector<std::shared_ptr<Action>> actions;
    for (int a = 0; a < actionCount; a++) {
      std::string actionName;
      std::vector<int32_t> actionArray;
      auto pStr = (int32_t*)stepArrayInfo.data() + a * actionStride;

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

    ActionResult actionResult;
    {
      // Release the GIL while executing actions so asyncio/other Python
      // threads are not blocked by the simulation. The mutex is only ever
      // acquired with the GIL released, so no thread waits on the mutex
      // while holding the GIL (avoids deadlock).
      py::gil_scoped_release release;
      std::lock_guard<std::recursive_mutex> lock(*mutex_);
      actionResult = player_->performActions(actions, updateTicks);
    }
    auto info = buildActionResultInfo(actionResult);
    auto rewards = gameProcess_->getAccumulatedRewards(player_->getId());
    return py::make_tuple(rewards, actionResult.terminated, info);
  }

  // Pure C++ action execution. Locks the player mutex but does NOT touch any
  // Python objects; call with the GIL released.
  ActionResult performActionsRaw(const std::string& actionName, const std::vector<int32_t>& actionArray, bool updateTicks) {
    std::lock_guard<std::recursive_mutex> lock(*mutex_);
    auto action = buildAction(actionName, actionArray);
    if (action != nullptr) {
      return player_->performActions({action}, updateTicks);
    }
    return player_->performActions({}, updateTicks);
  }

  // Record actions for the next game-clock tick: same array format as step_multi (rows of
  // [x, y, actionTypeIdx, actionId] / [actionTypeIdx, actionId] ...). Nothing runs until the clock ticks.
  void queueActions(py::ndarray<int32_t, py::ndim<2>> stepArray) {
    auto externalActionNames = gdyFactory_->getExternalActionNames();
    auto actionStride = stepArray.stride(0);
    auto actionArrayStride = stepArray.stride(1);
    std::vector<std::shared_ptr<Action>> actions;
    for (size_t a = 0; a < stepArray.shape(0); a++) {
      auto pStr = (int32_t*)stepArray.data() + a * actionStride;
      std::string actionName;
      std::vector<int32_t> actionArray;
      switch (stepArray.shape(1)) {
        case 2:
          actionName = externalActionNames.at(*(pStr + 0 * actionArrayStride));
          actionArray = {*(pStr + 1 * actionArrayStride)};
          break;
        case 4:
          actionName = externalActionNames.at(*(pStr + 2 * actionArrayStride));
          actionArray = {*(pStr + 0 * actionArrayStride), *(pStr + 1 * actionArrayStride), *(pStr + 3 * actionArrayStride)};
          break;
        default:
          throw std::invalid_argument(fmt::format("Invalid action size {0}, expected [x, y, actionTypeIdx, actionId] or [actionTypeIdx, actionId]", stepArray.shape(1)));
      }
      auto action = buildAction(actionName, actionArray);
      if (action != nullptr) actions.push_back(action);
    }
    py::gil_scoped_release release;
    std::lock_guard<std::recursive_mutex> lock(*mutex_);
    gameProcess_->queueActions(player_->getId(), actions);
  }

  // Record one named action ([x, y, actionId] or [actionId]) for the next tick
  void queueAction(std::string actionName, std::vector<int32_t> actionArray) {
    auto action = buildAction(actionName, actionArray);
    if (action == nullptr) return;
    py::gil_scoped_release release;
    std::lock_guard<std::recursive_mutex> lock(*mutex_);
    gameProcess_->queueActions(player_->getId(), {action});
  }

  py::tuple stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks) {
    if (gameProcess_ != nullptr && !gameProcess_->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    ActionResult actionResult;
    {
      // Release the GIL while executing actions so asyncio/other Python
      // threads are not blocked by the simulation.
      py::gil_scoped_release release;
      actionResult = performActionsRaw(actionName, actionArray, updateTicks);
    }

    auto info = buildActionResultInfo(actionResult);

    return py::make_tuple(actionResult.terminated, info);
  }

 private:
  std::shared_ptr<std::recursive_mutex> mutex_ = std::make_shared<std::recursive_mutex>();
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

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