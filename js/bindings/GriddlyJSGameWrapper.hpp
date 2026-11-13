#pragma once
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/GameState.hpp"
#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "GriddlyJSPlayerWrapper.hpp"

namespace e = emscripten;

class GriddlyJSGameWrapper {
 public:
  GriddlyJSGameWrapper(std::string globalObserverName, std::shared_ptr<griddly::GDYFactory> gdyFactory);

  std::shared_ptr<griddly::TurnBasedGameProcess> unwrapped();

  std::shared_ptr<GriddlyJSPlayerWrapper> registerPlayer(std::string playerName, std::string observerName);

  const uint32_t getActionTypeId(std::string actionName) const;

  void init();

  void release();

  void loadLevel(uint32_t levelId);

  void loadLevelString(std::string levelString);

  void reset();

  e::val getGlobalObservationDescription() const;

  e::val observe();
  // Zero-copy RGBA image (Uint8Array view over the SDL render buffer) + shape via getGlobalObservationDescription
  e::val observeImage();

  e::val stepParallel(e::val stepArray);

  // Several actions for one player in the same tick: each entry is [x, y, actionTypeIdx, actionId]
  // (or [actionTypeIdx, actionId] for avatar games). The game only advances when updateTicks is true,
  // so RTS UIs can submit every unit order of every player and tick once.
  e::val stepPlayer(uint32_t playerIdx, e::val actions, bool updateTicks);

  // {"x,y": [action names]} the player's objects can currently perform (internal actions excluded)
  e::val getAvailableActionNames(uint32_t playerId) const;

  // {action name: [valid action ids]} for the object at (x, y)
  e::val getAvailableActionIds(int32_t x, int32_t y, e::val actionNames) const;

  uint32_t getWidth() const;

  uint32_t getHeight() const;

  e::val getState() const;

  std::vector<std::string> getGlobalVariableNames() const;

  e::val getObjectVariableMap() const;

  e::val getGlobalVariables(std::vector<std::string> variables);

  std::vector<std::string> getObjectNames();

  std::vector<std::string> getObjectVariableNames();

  void seedRandomGenerator(uint32_t seed);

 private:
  std::shared_ptr<griddly::TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<griddly::GDYFactory> gdyFactory_;
  uint32_t playerCount_ = 0;
  std::vector<std::shared_ptr<GriddlyJSPlayerWrapper>> players_;
};