#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "GDY/GDYFactory.hpp"
#include "GDY/TerminationHandler.hpp"
#include "Grid.hpp"
#include "Observers/Observer.hpp"

namespace griddly {

class Player;

struct ActionResult {
  std::unordered_map<uint32_t, TerminationState> playerStates;
  bool terminated;
};

// Result of running queued actions on the game clock
struct TickResult {
  uint32_t ticks = 0;           // ticks run by this call
  uint32_t actions = 0;         // queued actions executed
  ActionResult result{{}, false};
};

struct SortObjects {
  inline bool operator()(const std::shared_ptr<ObjectRules>& a, const std::shared_ptr<ObjectRules>& b) {
    auto loca = a->getLocation().x * 1000000 + a->getLocation().y;
    auto locb = b->getLocation().x * 1000000 + b->getLocation().y;
    if (loca == locb) {
      if (a->getPlayerId() == b->getPlayerId()) {
        return a->getObjectName() < b->getObjectName();
      }
      return a->getPlayerId() < b->getPlayerId();
    }
    return loca < locb;
  }
};

// TODO: oh christ
class SortDelayedActionData {
 public:
  bool operator()(const DelayedActionData& a, const DelayedActionData& b) {
    if (a.priority == b.priority) {
      if (a.sourceObjectIdx == b.sourceObjectIdx) {
        if (a.playerId == b.playerId) {
          if (a.originatingPlayerId == b.originatingPlayerId) {
            if (a.orientationVector[0] == b.orientationVector[0]) {
              if (a.orientationVector[1] == b.orientationVector[1]) {
                if (a.vectorToDest[0] == b.vectorToDest[0]) {
                  if (a.vectorToDest[1] == b.vectorToDest[1]) {
                    return a.actionName < b.actionName;
                  }
                  return a.vectorToDest[1] < b.vectorToDest[1];
                }
                return a.vectorToDest[0] < b.vectorToDest[0];
              }
              return a.orientationVector[1] < b.orientationVector[1];
            }
            return a.orientationVector[0] < b.orientationVector[0];
          }
          return a.originatingPlayerId < b.originatingPlayerId;
        }
        return a.playerId < b.playerId;
      }
      return a.sourceObjectIdx < b.sourceObjectIdx;
    }
    return a.priority < b.priority;
  }
};

class GameProcess : public std::enable_shared_from_this<GameProcess> {
 public:
  GameProcess(std::string globalObserverName,
              std::shared_ptr<GDYFactory> gdyFactory,
              std::shared_ptr<Grid> grid);

  virtual ActionResult performActions(
      uint32_t playerId, std::vector<std::shared_ptr<Action>> actions,
      bool updateTicks = true) = 0;

  virtual void addPlayer(std::shared_ptr<Player> player);

  // Set the level by its id in the GDY description
  virtual void setLevel(uint32_t levelId);

  // Use a custom level string
  virtual void setLevel(std::string levelString);

  virtual void init(bool isCloned = false);

  virtual void reset();

  // Instant reset: restore the ECS state block captured right after the level
  // was loaded (single memcpy) instead of regenerating the level.
  virtual void resetFromSnapshot();

  bool isInitialized() const;

  virtual int32_t getAccumulatedRewards(uint32_t playerId);

  virtual std::string getProcessName() const;

  std::shared_ptr<Grid> getGrid();
  std::shared_ptr<Observer> getObserver();

  // An extra named observer seen from a player's perspective (player ids start at 1), e.g. a
  // fog-of-war mini-map next to the player's main view. Created on first use and reset with the game.
  std::shared_ptr<Observer> getPlayerObserver(uint32_t playerId, const std::string& observerName);

  virtual std::unordered_map<glm::ivec2, std::unordered_set<std::string>>
  getAvailableActionNames(uint32_t playerId) const;

  virtual std::vector<uint32_t> getAvailableActionIdsAtLocation(
      glm::ivec2 location, std::string actionName) const;

  virtual const GameState getGameState();

  // The game state as one player may know it (Environment.Player.FogOfWar): their own objects,
  // terrain and neutral objects on cells they have explored, other players' objects only while in
  // sight, and only the per-player global variables of that player (other players' values are 0).
  // Without fog of war this is the full state. No delayed actions are included.
  virtual const GameState getPlayerGameState(uint32_t playerId);

  // Fog of war cells of a player (0 unseen, 1 explored, 2 visible); empty without fog of war
  const std::vector<uint8_t>& getPlayerVisibility(uint32_t playerId) const;

  // ----- Game clock (Environment.TickRate) -----
  // Players record their actions between ticks; each tick runs everything recorded since the last
  // one (players take turns going first, actions within a player are shuffled by the grid) and then
  // advances time once, so all players' orders land in the same tick.

  // Record actions for the next tick (kept in order; the grid resolves them like any other step)
  void queueActions(uint32_t playerId, const std::vector<std::shared_ptr<Action>>& actions);
  // Drop everything recorded for a player (0 = all players)
  void clearQueuedActions(uint32_t playerId = 0);
  size_t getQueuedActionCount(uint32_t playerId = 0) const;

  // Run one tick now with the recorded actions (works paused or with TickRate 0)
  TickResult tick();

  // Real-time play: let `seconds` of wall time pass and run the ticks that became due at the tick
  // rate (none when paused, TickRate 0, or the game has ended). At most `maxTicks` per call: the
  // default of 1 gives players a chance to record new actions between ticks (a late frame catches
  // up over the next calls instead of running ticks with nobody's orders).
  TickResult advanceTime(double seconds, uint32_t maxTicks = 1);

  // Ticks per second; starts from Environment.TickRate (0 = no clock, tick() only)
  void setTickRate(double ticksPerSecond);
  double getTickRate() const { return tickRate_; }
  void setPaused(bool paused);
  bool isPaused() const { return paused_; }
  bool isTerminated() const { return requiresReset_; }
  virtual const GameStateMapping& getGameStateMapping() const;

  virtual uint32_t getNumPlayers() const;

  virtual void seedRandomGenerator(uint32_t seed) = 0;

  void release();

  virtual ~GameProcess() = default;

 protected:
  virtual void setLevelGenerator(
      std::shared_ptr<LevelGenerator> levelGenerator);
  virtual std::shared_ptr<LevelGenerator> getLevelGenerator() const;

  std::vector<std::shared_ptr<Player>> players_;
  std::shared_ptr<Grid> grid_;
  std::shared_ptr<GDYFactory> gdyFactory_;
  std::shared_ptr<TerminationHandler> terminationHandler_;

  // Game process can have its own observer so we can monitor the game
  // regardless of the player
  std::string globalObserverName_;
  std::shared_ptr<Observer> observer_;

  // extra per-player observers, keyed by (player id, observer name)
  std::map<std::pair<uint32_t, std::string>, std::shared_ptr<Observer>> extraPlayerObservers_;

  // A level generator used to reset the environment
  std::shared_ptr<LevelGenerator> levelGenerator_;

  bool isInitialized_ = false;

  // Should the game process reset itself or rely on external reset
  bool autoReset_ = false;

  // track whether this environment has finished or not, if it requires a reset,
  // we can reset it
  bool requiresReset_ = true;

  // Tracks the rewards currently accumulated per player
  std::unordered_map<uint32_t, int32_t> accumulatedRewards_;

  // game clock
  std::map<uint32_t, std::vector<std::shared_ptr<Action>>> queuedActions_;
  double tickRate_ = 0.0;
  double tickAccumulator_ = 0.0;
  bool paused_ = false;

 private:
  void generateStateHash(GameState& gameState);
  void resetObservers();
};
}  // namespace griddly
