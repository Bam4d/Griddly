#pragma once

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Objects/Object.hpp"
#include "GDY/Actions/Action.hpp"
#include "LevelGenerators/LevelGenerator.hpp"
#include "Util/util.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

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

class DelayedActionQueueItem;

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  virtual void resetMap(uint32_t height, uint32_t width);
  virtual void resetGlobalVariables(std::unordered_map<std::string, int32_t> globalVariableDefinitions);

  virtual std::vector<int> performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions);
  virtual int32_t executeAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual void delayAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual std::unordered_map<uint32_t, int32_t> update();

  virtual VectorPriorityQueue<DelayedActionQueueItem> getDelayedActions();

  virtual bool updateLocation(std::shared_ptr<Object> object, glm::ivec2 previousLocation, glm::ivec2 newLocation);

  // Mark a particular location to be repainted
  virtual bool invalidateLocation(glm::ivec2 location);

  virtual std::unordered_set<glm::ivec2> getUpdatedLocations() const;

  virtual uint32_t getWidth() const;
  virtual uint32_t getHeight() const;

  virtual std::shared_ptr<int32_t> getTickCount() const;
  virtual void setTickCount(int32_t tickCount);

  virtual void initObject(std::string objectName);
  virtual void addObject(uint32_t playerId, glm::ivec2 location, std::shared_ptr<Object> object);
  virtual bool removeObject(std::shared_ptr<Object> object);

  virtual std::unordered_set<std::shared_ptr<Object>>& getObjects();

  /**
   * Gets all the objects at a certain location
   */
  virtual TileObjects getObjectsAt(glm::ivec2 location) const;

  /**
   * Gets the object with the highest Z index at a certain tile location
   */
  virtual std::shared_ptr<Object> getObject(glm::ivec2 location) const;

  /**
   * Gets the number of unique objects in the grid
   */
  virtual uint32_t getUniqueObjectCount() const;

  virtual std::unordered_map<uint32_t, std::shared_ptr<int32_t>> getObjectCounter(std::string objectName);

  virtual std::unordered_map<std::string, std::shared_ptr<int32_t>> getGlobalVariables() const;

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
  std::unordered_set<glm::ivec2> updatedLocations_;

  std::unordered_set<std::string> objectNames_;
  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<glm::ivec2, TileObjects> occupiedLocations_;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> objectCounters_;
  std::unordered_map<std::string, std::shared_ptr<int32_t>> globalVariables_;

  // A priority queue of actions that are delayed in time (time is measured in game ticks)
  VectorPriorityQueue<DelayedActionQueueItem> delayedActions_;

  bool recordEvents_ = false;
  std::vector<GridEvent> eventHistory_;
};

}  // namespace griddly