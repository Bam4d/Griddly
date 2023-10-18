#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "Player.cpp"
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

struct InfoAndTruncated {
  py::dict info;
  bool truncated;
};

class Py_GameProcess {
 public:
  Py_GameProcess(std::string globalObserverName, std::shared_ptr<GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
    std::shared_ptr<Grid> grid = std::make_shared<Grid>(Grid());
    gameProcess_ = std::make_shared<TurnBasedGameProcess>(TurnBasedGameProcess(globalObserverName, gdyFactory, grid));
    spdlog::debug("Created game process wrapper");
  }

  Py_GameProcess(std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<TurnBasedGameProcess> gameProcess)
      : gdyFactory_(gdyFactory),
        gameProcess_(gameProcess) {
    spdlog::debug("Cloned game process wrapper");
  }

  std::shared_ptr<TurnBasedGameProcess> unwrapped() {
    return gameProcess_;
  }

  std::shared_ptr<Py_Player> registerPlayer(std::string playerName, std::string observerName) {
    // auto observerName = Observer::getDefaultObserverName(observerType);
    auto nextPlayerId = ++playerCount_;

    auto player = std::make_shared<Py_Player>(Py_Player(nextPlayerId, playerName, observerName, gdyFactory_, gameProcess_));
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

  py::dict getGlobalObservationDescription() const {
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

    if (stepArrayInfo.ndim != 3) {
      auto error = fmt::format("Invalid number of dimensions {0}, must be 3. [P,N,A] (Num Players, Number of actions, Action Size)", stepArrayInfo.ndim);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto playerStride = stepArrayInfo.strides[0] / sizeof(int32_t);
    auto numActionStride = stepArrayInfo.strides[1] / sizeof(int32_t);
    auto actionArrayStride = stepArrayInfo.strides[2] / sizeof(int32_t);

    spdlog::debug("Strides: {0}, {1}, {2}", playerStride, numActionStride, actionArrayStride);

    auto playerSize = stepArrayInfo.shape[0];
    auto numActionSize = stepArrayInfo.shape[1];
    auto actionSize = stepArrayInfo.shape[2];

    spdlog::debug("Shapes: {0}, {1}, {2}", playerSize, numActionSize, actionSize);

    if (playerSize != playerCount_) {
      auto error = fmt::format("The number of players {0} does not match the first dimension of the parallel action.", playerCount_);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto externalActionNames = gdyFactory_->getExternalActionNames();

    std::vector<int32_t> playerRewards{};
    bool terminated = false;
    bool truncated = false;
    py::dict info{};

    std::vector<uint32_t> playerIdx;

    for (uint32_t p = 0; p < playerSize; p++) {
      playerIdx.push_back(p);
    }

    std::shuffle(playerIdx.begin(), playerIdx.end(), gameProcess_->getGrid()->getRandomGenerator()->getEngine());

    for (int i = 0; i < playerSize; i++) {
      auto p = playerIdx[i];
      auto pStr = p * playerStride;
      bool lastPlayer = i == (playerSize - 1);
      std::vector<std::shared_ptr<Action>> actions{};
      for (int n = 0; n < numActionSize; n++) {
        std::string actionName;
        std::vector<int32_t> actionArray;
        auto nStr = n * numActionStride;
        auto offset = (int32_t*)stepArrayInfo.ptr + pStr + nStr;

        spdlog::debug("Player {0} action {1} offset {2}", p, n, pStr + nStr);

        switch (actionSize) {
          case 1:
            actionName = externalActionNames.at(0);
            actionArray.push_back(*(offset + 0 * actionArrayStride));
            break;
          case 2:
            actionName = externalActionNames.at(*(offset + 0 * actionArrayStride));
            actionArray.push_back(*(offset + 1 * actionArrayStride));
            break;
          case 3:
            actionName = externalActionNames.at(0);
            actionArray.push_back(*(offset + 0 * actionArrayStride));
            actionArray.push_back(*(offset + 1 * actionArrayStride));
            actionArray.push_back(*(offset + 2 * actionArrayStride));
            break;
          case 4:
            actionArray.push_back(*(offset + 0 * actionArrayStride));
            actionArray.push_back(*(offset + 1 * actionArrayStride));
            actionName = externalActionNames.at(*(offset + 2 * actionArrayStride));
            actionArray.push_back(*(offset + 3 * actionArrayStride));
            break;
          default: {
            auto error = fmt::format("Invalid action size, {0}", actionSize);
            spdlog::error(error);
            throw std::invalid_argument(error);
          }
        }

        spdlog::debug("Creating action for player {0} with name {1}", p, actionName);
        auto action = buildAction(players_[p]->unwrapped(), actionName, actionArray);
        if (action != nullptr) {
          actions.push_back(action);
        }
      }
      auto actionResult = players_[p]->performActions(actions, lastPlayer);
      terminated = actionResult.terminated;
      truncated = actionResult.truncated;

      if (lastPlayer) {
        spdlog::debug("Last player, updating ticks");
        auto info_and_truncated = buildInfo(actionResult);
        info = info_and_truncated.info;
        truncated = info_and_truncated.truncated;
      }
    }

    for (int p = 0; p < playerSize; p++) {
      playerRewards.push_back(gameProcess_->getAccumulatedRewards(p + 1));
    }

    return py::make_tuple(playerRewards, terminated, truncated, info);
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

  std::shared_ptr<Py_GameProcess> clone() {
    auto clonedGameProcess = gameProcess_->clone();
    auto clonedPyGameProcessWrapper = std::make_shared<Py_GameProcess>(Py_GameProcess(gdyFactory_, clonedGameProcess));

    return clonedPyGameProcessWrapper;
  }

  std::shared_ptr<Py_GameProcess> loadState(py::dict py_state) {
    const auto& stateMapping = gdyFactory_->getObjectGenerator()->getStateMapping();
    GameState gameState;

    gameState.tickCount = py_state["GameTicks"].cast<int32_t>();
    gameState.grid.height = py_state["Grid"]["Height"].cast<uint32_t>();
    gameState.grid.width = py_state["Grid"]["Width"].cast<uint32_t>();
    gameState.playerCount = py_state["PlayerCount"].cast<uint32_t>();

    // convert global variables
    py::dict py_globalVariables = py_state["GlobalVariables"].cast<py::dict>();
    gameState.globalData.resize(py_globalVariables.size());

    for (const auto& py_globalVariable : py_globalVariables) {
      const auto variableIndex = stateMapping.globalVariableNameToIdx.at(py_globalVariable.first.cast<std::string>());
      gameState.globalData[variableIndex] = py_globalVariable.second.cast<std::vector<int32_t>>();
    }

    // convert objects
    py::list py_objects = py_state["Objects"].cast<py::list>();
    gameState.objectData.resize(py_objects.size());

    spdlog::debug("Loading {0} objects", py_objects.size());

    // TODO: Assuming the order of the objects here is consistent with the indexes in the delayed actions...
    // might have to use ids here instead maybe to make it order independent?
    for (uint32_t o = 0; o < py_objects.size(); o++) {
      const auto& py_objectHandle = py_objects[o];
      GameObjectData gameObjectData;

      auto py_object = py_objectHandle.cast<py::dict>();
      gameObjectData.name = py_object["Name"].cast<std::string>();

      spdlog::debug("Loading object and variables for {0}", gameObjectData.name);

      auto py_location = py_object["Location"].cast<std::vector<int32_t>>();
      const auto py_orientation = DiscreteOrientation::fromString(py_object["Orientation"].cast<std::string>());

      const auto variableIndex = stateMapping.objectVariableNameToIdx.at(gameObjectData.name);

      spdlog::debug("Loading {0} object variables", variableIndex.size());
      gameObjectData.variables.resize(variableIndex.size());

      gameObjectData.variables[GameStateMapping::xIdx] = py_location[0];
      gameObjectData.variables[GameStateMapping::yIdx] = py_location[1];
      gameObjectData.variables[GameStateMapping::dxIdx] = py_orientation[0];
      gameObjectData.variables[GameStateMapping::dyIdx] = py_orientation[1];

      gameObjectData.variables[GameStateMapping::playerIdIdx] = py_object["PlayerId"].cast<int32_t>();
      gameObjectData.variables[GameStateMapping::renderTileIdIdx] = py_object["RenderTileId"].cast<int32_t>();

      auto py_variables = py_object["Variables"].cast<py::dict>();
      spdlog::debug("Loading {0} custom object variables", py_variables.size());
      for (const auto& variable : py_variables) {
        gameObjectData.setVariableValue(variableIndex, variable.first.cast<std::string>(), variable.second.cast<int32_t>());
      }

      gameState.objectData[o] = gameObjectData;
    }

    // convert delayed actions
    py::list py_delayedActions = py_state["DelayedActions"].cast<py::list>();

    for (const auto& py_delayedActionData : py_delayedActions) {
      DelayedActionData delayedActionData;

      delayedActionData.priority = py_delayedActionData["Priority"].cast<uint32_t>();
      delayedActionData.playerId = py_delayedActionData["PlayerId"].cast<uint32_t>();
      delayedActionData.sourceObjectIdx = py_delayedActionData["SourceObjectIdx"].cast<uint32_t>();
      delayedActionData.actionName = py_delayedActionData["ActionName"].cast<std::string>();
      delayedActionData.originatingPlayerId = py_delayedActionData["OriginatingPlayerId"].cast<uint32_t>();

      auto py_vectorToDest = py_delayedActionData["VectorToDest"].cast<std::vector<int32_t>>();
      auto py_originatingVector = py_delayedActionData["Orientation"].cast<std::vector<int32_t>>();
      delayedActionData.vectorToDest = glm::ivec2(py_vectorToDest[0], py_vectorToDest[1]);
      delayedActionData.orientationVector = glm::ivec2(py_originatingVector[0], py_originatingVector[1]);

      gameState.delayedActionData.push_back(delayedActionData);
    }

    auto loadedGameProcess = gameProcess_->fromGameState(gameState);
    return std::make_shared<Py_GameProcess>(Py_GameProcess(gdyFactory_, loadedGameProcess));
  }

  py::dict getState() const {
    py::dict py_state;
    const auto& gameState = gameProcess_->getGameState();
    const auto& stateMapping = gdyFactory_->getObjectGenerator()->getStateMapping();

    py_state["GameTicks"] = gameState.tickCount;
    py_state["Hash"] = gameState.hash;
    py_state["PlayerCount"] = gameState.playerCount;

    py::dict py_grid;
    py_grid["Width"] = gameProcess_->getGrid()->getWidth();
    py_grid["Height"] = gameProcess_->getGrid()->getHeight();
    py_state["Grid"] = py_grid;

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

      py_objectInfo["PlayerId"] = gameObjectData.variables[GameStateMapping::playerIdIdx];
      py_objectInfo["RenderTileId"] = gameObjectData.variables[GameStateMapping::renderTileIdIdx];
      py_objectInfo["Variables"] = py_objectVariables;

      py_objects.append(py_objectInfo);
    }

    py_state["Objects"] = py_objects;

    py::list py_delayed_actions;

    for (const auto& delayedActionData : gameState.delayedActionData) {
      py::dict py_delayedActionInfo;

      py_delayedActionInfo["Priority"] = delayedActionData.priority;
      py_delayedActionInfo["PlayerId"] = delayedActionData.playerId;
      py_delayedActionInfo["SourceObjectIdx"] = delayedActionData.sourceObjectIdx;
      py_delayedActionInfo["ActionName"] = delayedActionData.actionName;
      py_delayedActionInfo["VectorToDest"] = py::cast(std::vector<int32_t>{delayedActionData.vectorToDest.x, delayedActionData.vectorToDest.y});
      py_delayedActionInfo["Orientation"] = py::cast(std::vector<int32_t>{delayedActionData.orientationVector.x, delayedActionData.orientationVector.y});
      py_delayedActionInfo["OriginatingPlayerId"] = delayedActionData.originatingPlayerId;

      py_delayed_actions.append(py_delayedActionInfo);
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
  std::vector<std::shared_ptr<Py_Player>> players_;

  InfoAndTruncated buildInfo(ActionResult actionResult) {
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

    return {py_info, truncated};
  }

  std::shared_ptr<Action> buildAction(std::shared_ptr<Player> player, std::string actionName, std::vector<int32_t> actionArray) {
    const auto& actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
    auto playerAvatar = player->getAvatar();
    auto playerId = player->getId();

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