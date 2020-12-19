#pragma once

#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"

namespace griddly {
class Py_GameWrapper {
 public:
  Py_GameWrapper(ObserverType globalObserverType, std::shared_ptr<GDYFactory> gdyFactory)
      : gdyFactory_(gdyFactory),
        gameProcess_(std::shared_ptr<TurnBasedGameProcess>(
            new TurnBasedGameProcess(
                globalObserverType,
                gdyFactory,
                std::shared_ptr<Grid>(new Grid())))) {
    spdlog::debug("Created game process wrapper");
  }

  Py_GameWrapper(std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<TurnBasedGameProcess> gameProcess)
      : gdyFactory_(gdyFactory),
        gameProcess_(gameProcess) {
    spdlog::debug("Cloned game process wrapper");
  }

  std::shared_ptr<TurnBasedGameProcess> unwrapped() {
    return gameProcess_;
  }

  std::shared_ptr<Py_StepPlayerWrapper> registerPlayer(std::string playerName, ObserverType observerType) {
    auto observer = gdyFactory_->createObserver(gameProcess_->getGrid(), observerType);

    auto nextPlayerId = ++playerCount_;
    auto player = std::shared_ptr<Py_StepPlayerWrapper>(new Py_StepPlayerWrapper(nextPlayerId, playerName, observer, gdyFactory_, gameProcess_));
    players_.push_back(player);
    gameProcess_->addPlayer(player->unwrapped());
    return player;
  }

  uint32_t getNumPlayers() const {
    return gameProcess_->getNumPlayers();
  }

  py::dict getAvailableActionNames(int playerId) const {
    auto availableActionNames = gameProcess_->getAvailableActionNames(playerId);

    py::dict py_availableActionNames;
    for (auto availableActionNamesPair : availableActionNames) {
      auto location = availableActionNamesPair.first;
      auto actionNames = availableActionNamesPair.second;

      py::tuple locationKeyTuple = py::cast(std::vector<int32_t>{location.x, location.y});
      py_availableActionNames[locationKeyTuple] = actionNames;
    }

    return py_availableActionNames;
  }

  py::dict getAvailableActionIds(std::vector<int32_t> location, std::vector<std::string> actionNames) {
    py::dict py_availableActionIds;
    for (auto actionName : actionNames) {
      auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
      if (actionInputsDefinitions.find(actionName) != actionInputsDefinitions.end()) {
        auto locationVec = glm::ivec2{location[0], location[1]};
        auto actionIdsForName = gameProcess_->getAvailableActionIdsAtLocation(locationVec, actionName);

        py_availableActionIds[actionName.c_str()] = py::cast(actionIdsForName);
      }
    }

    return py_availableActionIds;
  }

  void init(bool isCloned) {
    gameProcess_->init(isCloned);
  }

  void loadLevel(uint32_t levelId) {
    gameProcess_->setLevel(levelId);
  }

  void loadLevelString(std::string levelString) {
    gameProcess_->setLevel(levelString);
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> reset() {
    auto observation = gameProcess_->reset();
    if (observation != nullptr) {
      auto observer = gameProcess_->getObserver();
      return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), std::move(observation)));
    }

    return nullptr;
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {
    auto observer = gameProcess_->getObserver();

    if (observer == nullptr) {
      throw std::invalid_argument("No global observer configured");
    }

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), gameProcess_->observe()));
  }

  py::list stepParallel(py::buffer stepArray) {
    auto stepArrayInfo = stepArray.request();

    if (stepArrayInfo.format != py::format_descriptor<int32_t>::format()) {
      auto error = fmt::format("Invalid data type, must be integers.");
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    switch (stepArrayInfo.ndim) {
      case 1: {
      } break;
      case 2: {
        auto playerStride = stepArrayInfo.strides[0];
        auto actionArrayStride = stepArrayInfo.strides[1];

        auto playerSize = stepArrayInfo.shape[0];
        auto actionSize = stepArrayInfo.shape[1];

        if (playerSize != playerCount_) {
          auto error = fmt::format("The number of players {0} does not match the first dimension of the parallel action.", playerCount_);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }

        py::list stepResults;
        std::vector<int32_t> actionArray;
        for (int p = 0; p < playerSize; p++) {
          auto pStr = p * playerStride;
          for (int a = 0; a < actionSize; a++) {
            actionArray.push_back(*((int32_t*)stepArrayInfo.ptr + pStr + a * actionArrayStride));
          }
          // stepResults.insert(0, players_[p]->stepMulti({actionArray}, p==(playerSize-1)));
        }

        return stepResults;

      } break;
      default: {
        auto error = fmt::format("Please provide an array with at least 1 dimension.");
        spdlog::error(error);
        throw std::invalid_argument(error);
      } break;
    }
  }

  std::array<uint32_t, 2> getTileSize() const {
    auto tileSize = gameProcess_->getObserver()->getTileSize();
    return {(uint32_t)tileSize[0], (uint32_t)tileSize[1]};
  }

  void enableHistory(bool enable) {
    gameProcess_->getGrid()->enableHistory(enable);
  }

  uint32_t getWidth() const {
    return gameProcess_->getGrid()->getWidth();
  }

  uint32_t getHeight() const {
    return gameProcess_->getGrid()->getHeight();
  }

  // force release of resources for vulkan etc
  void release() {
    gameProcess_->release();
  }

  std::shared_ptr<Py_GameWrapper> clone() {
    auto clonedGameProcess = gameProcess_->clone();

    auto clonedPyGameProcessWrapper = std::shared_ptr<Py_GameWrapper>(
        new Py_GameWrapper(
            gdyFactory_,
            clonedGameProcess));

    return clonedPyGameProcessWrapper;
  }

  py::dict getState() const {
    py::dict py_state;
    auto state = gameProcess_->getState();

    py_state["GameTicks"] = state.gameTicks;

    py::dict py_globalVariables;
    for (auto varIt : state.globalVariables) {
      py_globalVariables[varIt.first.c_str()] = varIt.second;
    }

    py_state["GlobalVariables"] = py_globalVariables;

    py::list py_objects;
    for (auto objectInfo : state.objectInfo) {
      py::dict py_objectInfo;
      py::dict py_objectVariables;
      for (auto varIt : objectInfo.variables) {
        py_objectVariables[varIt.first.c_str()] = varIt.second;
      }

      py_objectInfo["Name"] = objectInfo.name;
      py_objectInfo["Location"] = py::cast(std::vector<int32_t>{
          objectInfo.location.x,
          objectInfo.location.y});
      py_objectInfo["PlayerId"] = objectInfo.playerId;
      py_objectInfo["Variables"] = py_objectVariables;

      py_objects.insert(0, py_objectInfo);
    }

    py_state["Objects"] = py_objects;

    return py_state;
  }

 private:
  const std::shared_ptr<TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  uint32_t playerCount_ = 0;
  std::vector<std::shared_ptr<Py_StepPlayerWrapper>> players_;
};
}  // namespace griddly