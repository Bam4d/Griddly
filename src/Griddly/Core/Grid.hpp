#pragma once

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/Object.hpp"
#include "LevelGenerators/LevelGenerator.hpp"
#include "Util/util.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define TileObjects std::map<uint32_t, std::shared_ptr<Object>>

namespace griddly {

// Structure to hold information about the events that have happened at each time step
struct GridEvent {
  uint32_t playerId;
  std::string actionName;
  uint32_t tick = 0;
  int32_t reward = 0;
  uint32_t delay = 0;

  std::string sourceObjectName;
  std::string destObjectName;

  uint32_t sourceObjectPlayerId = 0;
  uint32_t destinationObjectPlayerId = 0;

  glm::vec2 sourceLocation;
  glm::vec2 destLocation;
};

struct GlobalVariableDefinition {
  int32_t initialValue = 0;
  bool perPlayer = false;
};

class DelayedActionQueueItem;

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  virtual void setPlayerCount(uint32_t playerCount);
  virtual void resetMap(uint32_t height, uint32_t width);
  virtual void resetGlobalVariables(std::unordered_map<std::string, GlobalVariableDefinition> globalVariableDefinitions);
  virtual void setGlobalVariables(std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>> globalVariableDefinitions);

  virtual std::vector<int> performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions);
  virtual int32_t executeAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual void delayAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual std::unordered_map<uint32_t, int32_t> update();

  virtual VectorPriorityQueue<DelayedActionQueueItem> getDelayedActions();

  virtual bool updateLocation(std::shared_ptr<Object> object, glm::ivec2 previousLocation, glm::ivec2 newLocation);

  // Mark a particular location to be repainted
  virtual bool invalidateLocation(glm::ivec2 location);

  virtual const std::unordered_set<glm::ivec2>& getUpdatedLocations(uint32_t player) const;
  virtual void purgeUpdatedLocations(uint32_t player);

  virtual uint32_t getWidth() const;
  virtual uint32_t getHeight() const;

  virtual std::shared_ptr<int32_t> getTickCount() const;
  virtual void setTickCount(int32_t tickCount);

  virtual void initObject(std::string objectName);
  virtual void addObject(glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions = true);
  virtual bool removeObject(std::shared_ptr<Object> object);

  virtual const std::unordered_set<std::shared_ptr<Object>>& getObjects();

  /**
   * Gets all the objects at a certain location
   */
  virtual const TileObjects& getObjectsAt(glm::ivec2 location) const;

  /**
   * Gets the object with the highest Z index at a certain tile location
   */
  virtual std::shared_ptr<Object> getObject(glm::ivec2 location) const;

  /**
   * Gets the number of unique objects in the grid
   */
  virtual uint32_t getUniqueObjectCount() const;

  /**
   * Get a mapping of the avatar objects for players in the environment
   */
  virtual std::unordered_map<uint32_t, std::shared_ptr<Object>> getPlayerAvatarObjects() const;

  virtual std::unordered_map<uint32_t, std::shared_ptr<int32_t>> getObjectCounter(std::string objectName);

  virtual std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> getGlobalVariables() const;

  virtual void enableHistory(bool enable);
  virtual std::vector<GridEvent> getHistory() const;
  virtual void purgeHistory();

 private:
  GridEvent buildGridEvent(std::shared_ptr<Action> action, uint32_t playerId, uint32_t tick);
  void recordGridEvent(GridEvent event, int32_t reward);

  int32_t executeAndRecord(uint32_t playerId, std::shared_ptr<Action> action);

  uint32_t height_;
  uint32_t width_;

  std::shared_ptr<int32_t> gameTicks_;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::vector<std::unordered_set<glm::ivec2>> updatedLocations_;

  std::unordered_set<std::string> objectNames_;
  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<glm::ivec2, TileObjects> occupiedLocations_;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> objectCounters_;
  std::unordered_map<uint32_t, std::shared_ptr<Object>> playerAvatars_;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables_;

  // return reference to this if there are no object in getObjectAt
  const std::map<uint32_t, std::shared_ptr<Object>> EMPTY_OBJECTS = {};
  const std::unordered_set<glm::ivec2> EMPTY_LOCATIONS = {};

  // A priority queue of actions that are delayed in time (time is measured in game ticks)
  VectorPriorityQueue<DelayedActionQueueItem> delayedActions_;

  // There is at least 1 player
  uint32_t playerCount_ = 1;

  bool recordEvents_ = false;
  std::vector<GridEvent> eventHistory_;
};

}  // namespace griddly