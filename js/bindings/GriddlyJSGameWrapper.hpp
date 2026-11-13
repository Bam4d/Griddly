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

  // A named render observer seen from a player's perspective (playerId starts at 1): fog of war,
  // camera and egocentric colours apply for that player. {pixels, shape} like observeImage.
  e::val observePlayerObserverImage(uint32_t playerId, std::string observerName);

  // What that observer showed in its last render, for mapping clicks to cells and hiding what the
  // player cannot see: {origin: [x, y] top-left cell, cells: [w, h] view size,
  // fog: Uint8Array (map width x height, 0 unseen / 1 explored / 2 visible) or null without fog}
  e::val getPlayerObserverState(uint32_t playerId, std::string observerName);

  e::val stepParallel(e::val stepArray);

  // Several actions for one player in the same tick: each entry is [x, y, actionTypeIdx, actionId]
  // (or [actionTypeIdx, actionId] for avatar games). The game only advances when updateTicks is true,
  // so RTS UIs can submit every unit order of every player and tick once.
  e::val stepPlayer(uint32_t playerIdx, e::val actions, bool updateTicks);

  // ----- Game clock (Environment.TickRate), in the engine -----
  // Record actions for a player (0-based index) to run on the next tick; same action format as
  // stepPlayer. Nothing runs until the clock ticks.
  void queueActions(uint32_t playerIdx, e::val actions);
  void clearQueuedActions(int32_t playerIdx);  // -1 = all players
  uint32_t getQueuedActionCount(int32_t playerIdx) const;
  // Run one tick now with everything recorded
  e::val tick();
  // Let `seconds` of wall time pass: runs the ticks that became due at the tick rate (at most
  // `maxTicks`, so players can record new actions between ticks)
  e::val advanceTime(double seconds, uint32_t maxTicks);
  void setTickRate(double ticksPerSecond);
  double getTickRate() const;
  void setPaused(bool paused);
  bool isPaused() const;

  // {"x,y": [action names]} the player's objects can currently perform (internal actions excluded)
  e::val getAvailableActionNames(uint32_t playerId) const;

  // {action name: [valid action ids]} for the object at (x, y)
  e::val getAvailableActionIds(int32_t x, int32_t y, e::val actionNames) const;

  uint32_t getWidth() const;

  uint32_t getHeight() const;

  e::val getState() const;

  // The state as one player may know it under Environment.Player.FogOfWar (own objects, explored
  // terrain, enemies only in sight, only that player's per-player variables) plus `visibility`:
  // a Uint8Array of the player's fog cells (0 unseen / 1 explored / 2 visible), null without fog.
  e::val getPlayerState(uint32_t playerId) const;

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

  e::val serializeState(const griddly::GameState& gameState) const;
  std::vector<std::shared_ptr<griddly::Action>> buildActions(uint32_t playerIdx, e::val actions);
  e::val tickResult(const griddly::TickResult& result);
};