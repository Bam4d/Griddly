#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "WrapperCommon.cpp"

namespace griddly {

class ValidActionNode {
 public:
  std::unordered_map<uint32_t, std::shared_ptr<ValidActionNode>> children;

  bool contains(uint32_t value) {
    return children.find(value) != children.end();
  }

  void add(uint32_t value) {
    children[value] = std::make_shared<ValidActionNode>(ValidActionNode());
  }

  static py::dict toPyDict(std::shared_ptr<ValidActionNode> node) {
    py::dict py_dict;
    for (auto child : node->children) {
      py_dict[py::cast(child.first)] = toPyDict(child.second);
    }

    return py_dict;
  }
};

class Py_GameWrapper {
 public:
  Py_GameWrapper(std::string globalObserverName, std::shared_ptr<GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
    std::shared_ptr<Grid> grid = std::make_shared<Grid>(Grid());
    gameProcess_ = std::make_shared<TurnBasedGameProcess>(TurnBasedGameProcess(globalObserverName, gdyFactory, grid));
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

  std::shared_ptr<Py_StepPlayerWrapper> registerPlayer(std::string playerName, std::string observerName) {
    // auto observerName = Observer::getDefaultObserverName(observerType);
    auto nextPlayerId = ++playerCount_;

    auto player = std::make_shared<Py_StepPlayerWrapper>(Py_StepPlayerWrapper(nextPlayerId, playerName, observerName, gdyFactory_, gameProcess_));
    players_.push_back(player);
    gameProcess_->addPlayer(player->unwrapped());
    return player;
  }

  const uint32_t getActionTypeId(std::string actionName) const {
    auto actionNames = gdyFactory_->getExternalActionNames();
    for (int i = 0; i < actionNames.size(); i++) {
      if (actionNames[i] == actionName) {
        return i;
      }
    }
    throw std::runtime_error("unregistered action");
  }

  std::vector<py::dict> buildValidActionTrees() const {
    std::vector<py::dict> valid_action_trees;
    auto externalActionNames = gdyFactory_->getExternalActionNames();
    spdlog::debug("Building tree, {0} actions", externalActionNames.size());
    for (int playerId = 1; playerId <= playerCount_; playerId++) {
      std::shared_ptr<ValidActionNode> node = std::make_shared<ValidActionNode>(ValidActionNode());
      for (auto actionNamesAtLocation : gameProcess_->getAvailableActionNames(playerId)) {
        auto location = actionNamesAtLocation.first;
        auto actionNames = actionNamesAtLocation.second;

        for (auto actionName : actionNames) {
          spdlog::debug("[{0}] available at location [{1}, {2}]", actionName, location.x, location.y);

          std::shared_ptr<ValidActionNode> treePtr = node;
          auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
          if (actionInputsDefinitions.find(actionName) != actionInputsDefinitions.end()) {
            auto locationVec = glm::ivec2{location[0], location[1]};
            auto actionIdsForName = gameProcess_->getAvailableActionIdsAtLocation(locationVec, actionName);

            spdlog::debug("{0} action ids available", actionIdsForName.size());

            if (actionIdsForName.size() > 0) {
              if (gdyFactory_->getAvatarObject().length() == 0) {
                auto py_x = locationVec[0];
                auto py_y = locationVec[1];
                if (!treePtr->contains(py_x)) {
                  treePtr->add(py_x);
                }

                treePtr = treePtr->children[py_x];

                if (!treePtr->contains(py_y)) {
                  treePtr->add(py_y);
                }

                treePtr = treePtr->children[py_y];
              }

              if (externalActionNames.size() > 1) {
                auto actionTypeId = getActionTypeId(actionName);
                if (!treePtr->contains(actionTypeId)) {
                  treePtr->add(actionTypeId);
                }

                treePtr = treePtr->children[actionTypeId];
              }

              for (auto id : actionIdsForName) {
                treePtr->add(id);
              }
              treePtr->add(0);
            }
          }
        }
      }
      valid_action_trees.push_back(ValidActionNode::toPyDict(node));
    }

    return valid_action_trees;
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
    spdlog::debug("Getting available action ids for location [{0},{1}]", location[0], location[1]);

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

  void reset() {
    gameProcess_->reset();
  }

  py::object getGlobalObservationDescription() const {
    return wrapObservationDescription(gameProcess_->getObserver());
  }

  py::object observe() {
    return wrapObservation(gameProcess_->getObserver());
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

    std::vector<int32_t> playerRewards{};
    bool terminated = false;
    py::dict info{};

    std::vector<uint32_t> playerIdx;

    for (uint32_t p = 0; p < playerSize; p++) {
      playerIdx.push_back(p);
    }

    std::shuffle(playerIdx.begin(), playerIdx.end(), gameProcess_->getGrid()->getRandomGenerator()->getEngine());

    for (int i = 0; i < playerSize; i++) {
      auto p = playerIdx[i];
      std::string actionName;
      std::vector<int32_t> actionArray;
      auto pStr = (int32_t*)stepArrayInfo.ptr + p * playerStride;

      bool lastPlayer = i == (playerSize - 1);

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

      // playerRewards.push_back(playerStepResult[0].cast<int32_t>());
      if (lastPlayer) {
        terminated = playerStepResult[0].cast<bool>();
        info = playerStepResult[1];
      }
    }

    for (int p = 0; p < playerSize; p++) {
      playerRewards.push_back(gameProcess_->getAccumulatedRewards(p + 1));
    }

    return py::make_tuple(playerRewards, terminated, info);
  }

  std::array<uint32_t, 2> getTileSize() const {
    auto vulkanObserver = std::dynamic_pointer_cast<VulkanObserver>(gameProcess_->getObserver());
    if (vulkanObserver == nullptr) {
      return {0, 0};
    } else {
      auto tileSize = vulkanObserver->getTileSize();
      return {(uint32_t)tileSize[0], (uint32_t)tileSize[1]};
    }
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
    auto clonedPyGameProcessWrapper = std::make_shared<Py_GameWrapper>(Py_GameWrapper(gdyFactory_, clonedGameProcess));

    return clonedPyGameProcessWrapper;
  }

  py::dict getState() const {
    py::dict py_state;
    const auto& gameState = gameProcess_->getGameState();
    const auto& stateMapping = gdyFactory_->getObjectGenerator()->getStateMapping();

    py_state["GameTicks"] = gameState.tickCount;
    py_state["Hash"] = gameState.hash;

    py::dict py_globalVariables;
    for (auto varIdxIt : stateMapping.globalVariableNameToIdx) {
      py_globalVariables[varIdxIt.first.c_str()] = gameState.globalData[varIdxIt.second];
    }

    py_state["GlobalVariables"] = py_globalVariables;

    py::list py_objects;
    for (const auto& gameObjectData : gameState.objectData) {
      const auto& variableIndexes = gameObjectData.getVariableIndexes(stateMapping);

      py::dict py_objectInfo;
      py::dict py_objectVariables;
      for (const auto& varIdxIt : variableIndexes) {
        if (varIdxIt.first[0] != '_') {
          py_objectVariables[varIdxIt.first.c_str()] = gameObjectData.variables[varIdxIt.second];
        }
      }

      py_objectInfo["Name"] = gameObjectData.name;
      const auto& location = gameObjectData.getLocation(variableIndexes);
      py_objectInfo["Location"] = py::cast(std::vector<int32_t>{location.x, location.y});

      py_objectInfo["Orientation"] = gameObjectData.getOrientation(variableIndexes).getName();

      py_objectInfo["PlayerId"] = gameObjectData.getVariableValue(variableIndexes, "_playerId");
      py_objectInfo["Variables"] = py_objectVariables;

      py_objects.insert(0, py_objectInfo);
    }

    py_state["Objects"] = py_objects;

    py::list py_delayed_actions;

    for (const auto& delayedActionData : gameState.delayedActionData) {
      py::dict py_delayedActionInfo;

      py_delayedActionInfo["Priority"] = delayedActionData.priority;
      py_delayedActionInfo["SourceObjectIdx"] = delayedActionData.sourceObjectIdx;
      py_delayedActionInfo["ActionName"] = delayedActionData.actionName;
      py_delayedActionInfo["VectorToDest"] = py::cast(std::vector<int32_t>{delayedActionData.vectorToDest.x, delayedActionData.vectorToDest.y});
      py_delayedActionInfo["Orientation"] = py::cast(std::vector<int32_t>{delayedActionData.orientationVector.x, delayedActionData.orientationVector.y});
      py_delayedActionInfo["OriginatingPlayerId"] = delayedActionData.originatingPlayerId;

      py_delayed_actions.insert(0, py_delayedActionInfo);
    }

    py_state["DelayedActions"] = py_delayed_actions;

    return py_state;
  }

  std::vector<std::string> getGlobalVariableNames() const {
    std::vector<std::string> globalVariableNames;
    auto globalVariables = gameProcess_->getGrid()->getGlobalVariables();

    for (auto globalVariableIt : globalVariables) {
      globalVariableNames.push_back(globalVariableIt.first);
    }
    return globalVariableNames;
  }

  py::dict getObjectVariableMap() const {
    return py::cast(gameProcess_->getGrid()->getObjectVariableMap());
  }

  py::dict getGlobalVariables(std::vector<std::string> variables) const {
    py::dict py_globalVariables;
    auto globalVariables = gameProcess_->getGrid()->getGlobalVariables();

    for (auto variableNameIt : variables) {
      std::unordered_map<int32_t, int32_t> resolvedGlobalVariableMap;

      auto globalVariableMap = globalVariables[variableNameIt];

      for (auto playerVariableIt : globalVariableMap) {
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
      for (const auto& historyEvent : history) {
        py::dict py_event;

        py::dict rewards;
        for (auto& reward : historyEvent.rewards) {
          rewards[py::cast(reward.first)] = reward.second;
        }

        py_event["PlayerId"] = historyEvent.playerId;
        py_event["ActionName"] = historyEvent.actionName;
        py_event["Tick"] = historyEvent.tick;
        py_event["Rewards"] = rewards;
        py_event["Delay"] = historyEvent.delay;

        py_event["SourceObjectName"] = historyEvent.sourceObjectName;
        py_event["DestinationObjectName"] = historyEvent.destObjectName;

        py_event["SourceObjectPlayerId"] = historyEvent.sourceObjectPlayerId;
        py_event["DestinationObjectPlayerId"] = historyEvent.destinationObjectPlayerId;

        py_event["SourceLocation"] = std::array<uint32_t, 2>{(uint32_t)historyEvent.sourceLocation.x, (uint32_t)historyEvent.sourceLocation.y};
        py_event["DestinationLocation"] = std::array<uint32_t, 2>{(uint32_t)historyEvent.destLocation.x, (uint32_t)historyEvent.destLocation.y};

        py_events.push_back(py_event);
      }

      if (purge) {
        gameProcess_->getGrid()->purgeHistory();
      }
    }

    return py_events;
  }

  std::vector<std::string> getObjectNames() {
    return gameProcess_->getGrid()->getObjectNames();
  }

  std::vector<std::string> getObjectVariableNames() {
    return gameProcess_->getGrid()->getAllObjectVariableNames();
  }

  void seedRandomGenerator(uint32_t seed) {
    gameProcess_->seedRandomGenerator(seed);
  }

 private:
  std::shared_ptr<TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  uint32_t playerCount_ = 0;
  std::vector<std::shared_ptr<Py_StepPlayerWrapper>> players_;
};
}  // namespace griddly