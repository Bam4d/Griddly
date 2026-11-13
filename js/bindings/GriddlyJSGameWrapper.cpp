#include <emscripten/val.h>

#include <algorithm>
#include <memory>

#include "GriddlyJSGameWrapper.hpp"
#include "../../src/Griddly/Core/Observers/SDL/SDLObserver.hpp"
#include "../../src/Griddly/Core/Observers/SDLGridObserver.hpp"
#include "GriddlyJSWrapperCommon.cpp"

namespace e = emscripten;

GriddlyJSGameWrapper::GriddlyJSGameWrapper(std::string globalObserverName, std::shared_ptr<griddly::GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
  std::shared_ptr<griddly::Grid> grid = std::make_shared<griddly::Grid>(griddly::Grid());
  gameProcess_ = std::make_shared<griddly::TurnBasedGameProcess>(griddly::TurnBasedGameProcess(globalObserverName, gdyFactory, grid));
  spdlog::debug("Created game process wrapper");
}

std::shared_ptr<griddly::TurnBasedGameProcess> GriddlyJSGameWrapper::unwrapped() {
  return gameProcess_;
}

std::shared_ptr<GriddlyJSPlayerWrapper> GriddlyJSGameWrapper::registerPlayer(std::string playerName, std::string observerName) {
  auto nextPlayerId = ++playerCount_;

  auto player = std::make_shared<GriddlyJSPlayerWrapper>(GriddlyJSPlayerWrapper(nextPlayerId, playerName, observerName, gdyFactory_, gameProcess_));
  players_.push_back(player);
  gameProcess_->addPlayer(player->unwrapped());
  return player;
}

const uint32_t GriddlyJSGameWrapper::getActionTypeId(std::string actionName) const {
  auto actionNames = gdyFactory_->getExternalActionNames();
  for (int i = 0; i < actionNames.size(); i++) {
    if (actionNames[i] == actionName) {
      return i;
    }
  }
  throw std::runtime_error("unregistered action");
}

void GriddlyJSGameWrapper::init() {
  gameProcess_->init(false);
}

void GriddlyJSGameWrapper::release() {
  gameProcess_->release();
}

void GriddlyJSGameWrapper::loadLevel(uint32_t levelId) {
  gameProcess_->setLevel(levelId);
}

void GriddlyJSGameWrapper::loadLevelString(std::string levelString) {
  gameProcess_->setLevel(levelString);
}

void GriddlyJSGameWrapper::reset() {
  gameProcess_->reset();
}

e::val GriddlyJSGameWrapper::getGlobalObservationDescription() const {
  return wrapObservationDescription(gameProcess_->getObserver());
}

e::val GriddlyJSGameWrapper::observe() {
  return wrapObservation(gameProcess_->getObserver());
}

e::val GriddlyJSGameWrapper::observeImage() {
  return wrapObservationImage(gameProcess_->getObserver());
}

e::val GriddlyJSGameWrapper::observePlayerObserverImage(uint32_t playerId, std::string observerName) {
  auto observer = gameProcess_->getPlayerObserver(playerId, observerName);
  auto result = e::val::object();
  // render first: the shape is only final after the frame is drawn
  auto pixels = wrapObservationImage(observer);
  result.set("pixels", pixels);
  result.set("shape", wrapObservationDescription(observer)["Shape"]);
  return result;
}

e::val GriddlyJSGameWrapper::getPlayerObserverState(uint32_t playerId, std::string observerName) {
  auto observer = std::dynamic_pointer_cast<griddly::SDLGridObserver>(gameProcess_->getPlayerObserver(playerId, observerName));
  if (observer == nullptr) {
    throw std::invalid_argument(fmt::format("Observer {0} is not a Sprite2D / Block2D observer", observerName));
  }
  auto result = e::val::object();
  auto origin = e::val::array();
  origin.call<void>("push", observer->cameraOrigin().x);
  origin.call<void>("push", observer->cameraOrigin().y);
  auto cells = e::val::array();
  cells.call<void>("push", observer->viewCells().x);
  cells.call<void>("push", observer->viewCells().y);
  result.set("origin", origin);
  result.set("cells", cells);
  const auto& fog = observer->fogState();
  if (fog.empty()) {
    result.set("fog", e::val::null());
  } else {
    // copy: the observer's buffer is reused on the next render
    result.set("fog", e::val::global("Uint8Array").new_(e::typed_memory_view(fog.size(), fog.data())));
  }
  return result;
}

e::val GriddlyJSGameWrapper::stepParallel(e::val stepArray) {
  const auto& externalActionNames = gdyFactory_->getExternalActionNames();

  auto playerSize = stepArray["length"].as<uint32_t>();

  if (playerSize != playerCount_) {
    auto error = fmt::format("The number of players {0} does not match the first dimension of the parallel action.", playerCount_);
    spdlog::error(error);
    throw std::invalid_argument(error);
  }

  e::val playerRewards = e::val::array();
  bool terminated = false;
  e::val info = e::val::object();

  for (int p = 0; p < playerSize; p++) {
    auto playerStepArray = e::convertJSArrayToNumberVector<int32_t>(stepArray.call<e::val>("at", p));
    auto actionSize = playerStepArray.size();

    std::vector<int32_t> actionArray;

    std::string actionName;
    switch (actionSize) {
      case 1:
        actionName = externalActionNames.at(0);
        actionArray.push_back(playerStepArray[0]);
        break;
      case 2:
        actionName = externalActionNames.at(playerStepArray[0]);
        actionArray.push_back(playerStepArray[1]);
        break;
      case 3:
        actionName = externalActionNames.at(0);
        actionArray.push_back(playerStepArray[0]);
        actionArray.push_back(playerStepArray[1]);
        actionArray.push_back(playerStepArray[2]);
        break;
      case 4:
        actionName = externalActionNames.at(playerStepArray[2]);
        actionArray.push_back(playerStepArray[0]);
        actionArray.push_back(playerStepArray[1]);
        actionArray.push_back(playerStepArray[3]);
        break;
      default: {
        auto error = fmt::format("Invalid action size, {0}", actionSize);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
    }

    bool lastPlayer = p == (playerSize - 1);

    spdlog::debug("Player {0} action size: {1}, action: {2}", p, actionSize, actionName);

    auto playerStepResult = players_[p]->stepSingle(actionName, actionArray, lastPlayer);
    if (lastPlayer) {
      terminated = playerStepResult["terminated"].as<bool>();
      info = playerStepResult["info"];
    }
  }

  for (int p = 0; p < playerSize; p++) {
    playerRewards.call<void>("push", gameProcess_->getAccumulatedRewards(p + 1));
  }

  auto js_result = e::val::object();

  js_result.set("terminated", terminated);
  js_result.set("info", info);
  js_result.set("reward", playerRewards);

  return js_result;
}

e::val GriddlyJSGameWrapper::stepPlayer(uint32_t playerIdx, e::val actions, bool updateTicks) {
  if (playerIdx >= players_.size()) {
    throw std::invalid_argument(fmt::format("No player with index {0}", playerIdx));
  }
  const auto& externalActionNames = gdyFactory_->getExternalActionNames();
  auto count = actions["length"].as<uint32_t>();

  bool terminated = false;
  e::val info = e::val::object();
  auto collect = [&](e::val result) {
    if (result["terminated"].as<bool>()) {
      terminated = true;
      info = result["info"];
    }
  };

  for (uint32_t a = 0; a < count; a++) {
    auto action = e::convertJSArrayToNumberVector<int32_t>(actions.call<e::val>("at", a));
    std::string actionName;
    std::vector<int32_t> actionArray;
    if (action.size() == 4) {
      actionName = externalActionNames.at(action[2]);
      actionArray = {action[0], action[1], action[3]};
    } else if (action.size() == 2) {
      actionName = externalActionNames.at(action[0]);
      actionArray = {action[1]};
    } else {
      throw std::invalid_argument(fmt::format("Invalid action size {0}, expected [x, y, actionTypeIdx, actionId] or [actionTypeIdx, actionId]", action.size()));
    }
    bool last = a == count - 1;
    collect(players_[playerIdx]->stepSingle(actionName, actionArray, updateTicks && last));
  }

  if (count == 0 && updateTicks) {
    // nothing to do this tick: an empty action still advances the game
    collect(players_[playerIdx]->stepSingle(externalActionNames.at(0), {0, 0, 0}, true));
  }

  e::val playerRewards = e::val::array();
  for (uint32_t p = 0; p < players_.size(); p++) {
    playerRewards.call<void>("push", gameProcess_->getAccumulatedRewards(p + 1));
  }

  auto js_result = e::val::object();
  js_result.set("terminated", terminated);
  js_result.set("info", info);
  js_result.set("reward", playerRewards);
  return js_result;
}

std::vector<std::shared_ptr<griddly::Action>> GriddlyJSGameWrapper::buildActions(uint32_t playerIdx, e::val actions) {
  if (playerIdx >= players_.size()) {
    throw std::invalid_argument(fmt::format("No player with index {0}", playerIdx));
  }
  const auto& externalActionNames = gdyFactory_->getExternalActionNames();
  std::vector<std::shared_ptr<griddly::Action>> built;
  auto count = actions["length"].as<uint32_t>();
  for (uint32_t a = 0; a < count; a++) {
    auto action = e::convertJSArrayToNumberVector<int32_t>(actions.call<e::val>("at", a));
    std::shared_ptr<griddly::Action> engineAction;
    if (action.size() == 4) {
      engineAction = players_[playerIdx]->buildAction(externalActionNames.at(action[2]), {action[0], action[1], action[3]});
    } else if (action.size() == 2) {
      engineAction = players_[playerIdx]->buildAction(externalActionNames.at(action[0]), {action[1]});
    } else {
      throw std::invalid_argument(fmt::format("Invalid action size {0}, expected [x, y, actionTypeIdx, actionId] or [actionTypeIdx, actionId]", action.size()));
    }
    if (engineAction != nullptr) built.push_back(engineAction);
  }
  return built;
}

void GriddlyJSGameWrapper::queueActions(uint32_t playerIdx, e::val actions) {
  gameProcess_->queueActions(playerIdx + 1, buildActions(playerIdx, actions));
}

void GriddlyJSGameWrapper::clearQueuedActions(int32_t playerIdx) {
  gameProcess_->clearQueuedActions(playerIdx < 0 ? 0 : static_cast<uint32_t>(playerIdx) + 1);
}

uint32_t GriddlyJSGameWrapper::getQueuedActionCount(int32_t playerIdx) const {
  return static_cast<uint32_t>(gameProcess_->getQueuedActionCount(playerIdx < 0 ? 0 : static_cast<uint32_t>(playerIdx) + 1));
}

e::val GriddlyJSGameWrapper::tickResult(const griddly::TickResult& result) {
  e::val playerRewards = e::val::array();
  for (uint32_t p = 0; p < players_.size(); p++) {
    playerRewards.call<void>("push", gameProcess_->getAccumulatedRewards(p + 1));
  }
  auto js_result = e::val::object();
  js_result.set("ticks", result.ticks);
  js_result.set("actions", result.actions);
  js_result.set("terminated", result.result.terminated);
  js_result.set("info", GriddlyJSPlayerWrapper::buildInfo(result.result));
  js_result.set("reward", playerRewards);
  return js_result;
}

e::val GriddlyJSGameWrapper::tick() {
  return tickResult(gameProcess_->tick());
}

e::val GriddlyJSGameWrapper::advanceTime(double seconds, uint32_t maxTicks) {
  return tickResult(gameProcess_->advanceTime(seconds, maxTicks));
}

void GriddlyJSGameWrapper::setTickRate(double ticksPerSecond) {
  gameProcess_->setTickRate(ticksPerSecond);
}

double GriddlyJSGameWrapper::getTickRate() const {
  return gameProcess_->getTickRate();
}

void GriddlyJSGameWrapper::setPaused(bool paused) {
  gameProcess_->setPaused(paused);
}

bool GriddlyJSGameWrapper::isPaused() const {
  return gameProcess_->isPaused();
}

e::val GriddlyJSGameWrapper::getAvailableActionNames(uint32_t playerId) const {
  e::val js_available = e::val::object();
  for (const auto& [location, names] : gameProcess_->getAvailableActionNames(playerId)) {
    std::vector<std::string> sorted(names.begin(), names.end());
    std::sort(sorted.begin(), sorted.end());
    js_available.set(fmt::format("{0},{1}", location.x, location.y), e::val::array(sorted));
  }
  return js_available;
}

e::val GriddlyJSGameWrapper::getAvailableActionIds(int32_t x, int32_t y, e::val actionNames) const {
  e::val js_ids = e::val::object();
  const auto& actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
  for (const auto& actionName : e::vecFromJSArray<std::string>(actionNames)) {
    if (actionInputsDefinitions.find(actionName) == actionInputsDefinitions.end()) {
      continue;
    }
    auto ids = gameProcess_->getAvailableActionIdsAtLocation({x, y}, actionName);
    js_ids.set(actionName, e::val::array(ids));
  }
  return js_ids;
}

uint32_t GriddlyJSGameWrapper::getWidth() const {
  return gameProcess_->getGrid()->getWidth();
}

uint32_t GriddlyJSGameWrapper::getHeight() const {
  return gameProcess_->getGrid()->getHeight();
}

e::val GriddlyJSGameWrapper::serializeState(const griddly::GameState& gameState) const {
  e::val js_state = e::val::object();

  const auto& objectGenerator = gdyFactory_->getObjectGenerator();
  const auto& stateMapping = objectGenerator->getStateMapping();
  const auto& objectDefinitions = objectGenerator->getObjectDefinitions();

  js_state.set("gameTicks", gameState.tickCount);
  js_state.set("hash", gameState.hash);
  js_state.set("playerCount", gameState.playerCount);

  e::val js_grid = e::val::object();
  js_grid.set("width", gameProcess_->getGrid()->getWidth());
  js_grid.set("height", gameProcess_->getGrid()->getHeight());
  js_state.set("grid", js_grid);

  e::val js_globalVariables = e::val::object();

  for (auto varIdxIt : stateMapping.globalVariableNameToIdx) {
    js_globalVariables.set(varIdxIt.first.c_str(), e::val::array(gameState.globalData[varIdxIt.second]));
  }

  js_state.set("globalVariables", js_globalVariables);

  std::vector<e::val> objects_js{};
  for (auto gameObjectData : gameState.objectData) {
    e::val js_objectInfo = e::val::object();
    e::val js_objectVariables = e::val::object();

    const auto& variableIndexes = gameObjectData.getVariableIndexes(stateMapping);
    const auto& objectDefinition = objectDefinitions.at(gameObjectData.name);

    for (const auto& varIdxIt : variableIndexes) {
      if (varIdxIt.first[0] != '_') {
        js_objectVariables.set(varIdxIt.first.c_str(), gameObjectData.variables[varIdxIt.second]);
      }
    }

    js_objectInfo.set("id", gameObjectData.id);
    js_objectInfo.set("name", gameObjectData.name);
    const auto& location = gameObjectData.getLocation(variableIndexes);
    js_objectInfo.set("location", e::val::array(std::vector<int32_t>{location.x, location.y}));

    js_objectInfo.set("zidx", objectDefinition->zIdx);
    js_objectInfo.set("orientation", gameObjectData.getOrientation(variableIndexes).getName());
    js_objectInfo.set("playerId", gameObjectData.variables[griddly::GameStateMapping::playerIdIdx]);
    js_objectInfo.set("renderTileId", gameObjectData.variables[griddly::GameStateMapping::renderTileIdIdx]);
    js_objectInfo.set("variables", js_objectVariables);

    objects_js.push_back(js_objectInfo);
  }

  js_state.set("objects", e::val::array(objects_js));

  //TODO: delayed action serialization and loading of serialized state

  return js_state;
}

e::val GriddlyJSGameWrapper::getState() const {
  return serializeState(gameProcess_->getGameState());
}

e::val GriddlyJSGameWrapper::getPlayerState(uint32_t playerId) const {
  auto js_state = serializeState(gameProcess_->getPlayerGameState(playerId));
  const auto& visibility = gameProcess_->getPlayerVisibility(playerId);
  if (visibility.empty()) {
    js_state.set("visibility", e::val::null());
  } else {
    js_state.set("visibility", e::val::global("Uint8Array").new_(e::typed_memory_view(visibility.size(), visibility.data())));
  }
  return js_state;
}

std::vector<std::string> GriddlyJSGameWrapper::getGlobalVariableNames() const {
  std::vector<std::string> globalVariableNames;
  auto globalVariables = gameProcess_->getGrid()->getGlobalVariables();

  for (auto globalVariableIt : globalVariables) {
    globalVariableNames.push_back(globalVariableIt.first);
  }
  return globalVariableNames;
}

e::val GriddlyJSGameWrapper::getObjectVariableMap() const {
  auto test2 = e::val::object();
  auto test = e::val::object();
  test2.set("x", 5);

  test.set("X", test2);

  return test;
}

e::val GriddlyJSGameWrapper::getGlobalVariables(std::vector<std::string> variables) {
  e::val js_globalVariables = e::val::object();
  auto globalVariables = gameProcess_->getGrid()->getGlobalVariables();

  for (auto variableName : variables) {
    e::val js_resolvedGlobalVariableMap = e::val::object();

    auto globalVariableMap = globalVariables[variableName];

    for (auto playerVariableIt : globalVariableMap) {
      js_resolvedGlobalVariableMap.set(playerVariableIt.first, *playerVariableIt.second);
    }

    js_globalVariables.set(variableName, js_resolvedGlobalVariableMap);
  }
  return js_globalVariables;
}

std::vector<std::string> GriddlyJSGameWrapper::getObjectNames() {
  return gameProcess_->getGrid()->getObjectNames();
}

std::vector<std::string> GriddlyJSGameWrapper::getObjectVariableNames() {
  return gameProcess_->getGrid()->getAllObjectVariableNames();
}

void GriddlyJSGameWrapper::seedRandomGenerator(uint32_t seed) {
  gameProcess_->seedRandomGenerator(seed);
}