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

  py::tuple stepParallel(py::buffer stepArray) {


    auto stepArrayInfo = stepArray.request();
    if (stepArrayInfo.format != "l" && stepArrayInfo.format != "i") {
      auto error = fmt::format("Invalid data type {0}, must be an integer.", stepArrayInfo.format);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    spdlog::debug("Dims: {0}", stepArrayInfo.ndim);

    auto playerStride = stepArrayInfo.strides[0] / sizeof(int32_t);
    auto actionArrayStride = stepArrayInfo.strides[1] / sizeof(int32_t);

    auto playerSize = stepArrayInfo.shape[0];
    auto actionSize = stepArrayInfo.shape[1];

    if (playerSize != playerCount_) {
      auto error = fmt::format("The number of players {0} does not match the first dimension of the parallel action.", playerCount_);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto externalActionNames = gdyFactory_->getExternalActionNames();
    
    std::vector<int32_t> playerRewards;
    bool terminated;
    py::dict info;

    for (int p = 0; p < playerSize; p++) {
      std::string actionName;
      std::vector<int32_t> actionArray;
      auto pStr = (int32_t *)stepArrayInfo.ptr + p * playerStride;

      bool lastPlayer = p == (playerSize - 1);

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

      auto playerStepResult = players_[p]->stepSingle(actionName, actionArray, lastPlayer);

      playerRewards.push_back(playerStepResult[0].cast<int32_t>());
      if(lastPlayer) {
        terminated = playerStepResult[1].cast<bool>();
        info = playerStepResult[2];
      }
    }

    return py::make_tuple(playerRewards, terminated, info);
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
      py_objectInfo["Orientation"] = objectInfo.orientation.getName();
      py_objectInfo["PlayerId"] = objectInfo.playerId;
      py_objectInfo["Variables"] = py_objectVariables;

      py_objects.insert(0, py_objectInfo);
    }

    py_state["Objects"] = py_objects;

    return py_state;
  }

  py::dict getGlobalVariables(std::vector<std::string> variables) const {

    py::dict py_globalVariables;
    auto globalVariables = gameProcess_->getGrid()->getGlobalVariables();

    for (auto variableNameIt : variables) {
      std::unordered_map<int32_t, int32_t> resolvedGlobalVariableMap;

      auto globalVariableMap = globalVariables[variableNameIt];

      for(auto playerVariableIt : globalVariableMap) {
        resolvedGlobalVariableMap.insert({playerVariableIt.first, *playerVariableIt.second});
      }

      py_globalVariables[variableNameIt.c_str()] = resolvedGlobalVariableMap;
    }
    return py_globalVariables;
  }

  std::vector<py::dict> getHistory(bool purge) const {
    auto history = gameProcess_->getGrid()->getHistory();

    std::vector<py::dict> py_events;
    if (history.size() > 0) {
      for (auto historyEvent : history) {
        py::dict py_event;

        py_event["PlayerId"] = historyEvent.playerId;
        py_event["ActionName"] = historyEvent.actionName;
        py_event["Tick"] = historyEvent.tick;
        py_event["Reward"] = historyEvent.reward;
        py_event["Delay"] = historyEvent.delay;

        py_event["SourceObjectName"] = historyEvent.sourceObjectName;
        py_event["DestinationObjectName"] = historyEvent.destObjectName;

        py_event["SourceObjectPlayerId"] = historyEvent.sourceObjectPlayerId;
        py_event["DestinationObjectPlayerId"] = historyEvent.destinationObjectPlayerId;

        py_event["SourceLocation"] = std::array{historyEvent.sourceLocation.x, historyEvent.sourceLocation.y};
        py_event["DestinationLocation"] = std::array{historyEvent.destLocation.x, historyEvent.destLocation.y};

        py_events.push_back(py_event);
      }

      if (purge) {
        gameProcess_->getGrid()->purgeHistory();
      }
    }

    return py_events;
  }

 private:
  const std::shared_ptr<TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  uint32_t playerCount_ = 0;
  std::vector<std::shared_ptr<Py_StepPlayerWrapper>> players_;
};
}  // namespace griddly