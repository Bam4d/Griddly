#pragma once

#include <memory>
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

struct SortObjects {
  inline bool operator()(const std::shared_ptr<Object>& a, const std::shared_ptr<Object>& b) {
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

  bool isInitialized() const;

  virtual int32_t getAccumulatedRewards(uint32_t playerId);

  virtual std::string getProcessName() const;

  std::shared_ptr<Grid> getGrid();
  std::shared_ptr<Observer> getObserver();

  virtual std::unordered_map<glm::ivec2, std::unordered_set<std::string>>
  getAvailableActionNames(uint32_t playerId) const;

  virtual std::vector<uint32_t> getAvailableActionIdsAtLocation(
      glm::ivec2 location, std::string actionName) const;

  virtual const GameState getGameState();
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

 private:
  void generateStateHash(GameState& gameState);
  void resetObservers();
};
}  // namespace griddly
