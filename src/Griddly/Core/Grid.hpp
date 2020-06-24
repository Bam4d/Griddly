#pragma once

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/GridLocation.hpp"
#include "LevelGenerators/LevelGenerator.hpp"

#define TileObjects std::map<uint32_t, std::shared_ptr<Object>>

namespace griddly {

class DelayedActionQueueItem;

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  virtual void resetMap(uint32_t height, uint32_t width);
  virtual void resetGlobalVariables(std::unordered_map<std::string, int32_t> globalVariableDefinitions);

  virtual std::vector<int> performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions);
  virtual int executeAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual void delayAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual std::unordered_map<uint32_t, int32_t> update();

  virtual bool updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation);

  // Mark a particular location to be repainted
  virtual bool invalidateLocation(GridLocation location);

  virtual std::unordered_set<GridLocation, GridLocation::Hash> getUpdatedLocations() const;

  virtual uint32_t getWidth() const;
  virtual uint32_t getHeight() const;

  virtual std::shared_ptr<int32_t> getTickCount() const;

  virtual void initObject(uint32_t playerId, GridLocation location, std::shared_ptr<Object> object);
  virtual bool removeObject(std::shared_ptr<Object> object);

  virtual std::unordered_set<std::shared_ptr<Object>>& getObjects();

  /**
   * Gets all the objects at a certain location
   */
  virtual TileObjects getObjectsAt(GridLocation location) const;

  /**
   * Gets the object with the highest Z index at a certain tile location
   */
  virtual std::shared_ptr<Object> getObject(GridLocation location) const;

  /**
   * Gets the number of unique objects in the grid
   */
  virtual uint32_t getUniqueObjectCount() const;

  virtual std::unordered_map<uint32_t, std::shared_ptr<int32_t>> getObjectCounter(std::string objectName);

  virtual std::unordered_map<std::string, std::shared_ptr<int32_t>> getGlobalVariables() const;

 private:
  uint32_t height_;
  uint32_t width_;

  std::shared_ptr<int32_t> gameTicks_;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::unordered_set<GridLocation, GridLocation::Hash> updatedLocations_;

  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, TileObjects, GridLocation::Hash> occupiedLocations_;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> objectCounters_;
  std::unordered_map<std::string, std::shared_ptr<int32_t>> globalVariables_;

  // A priority queue of actions that are delayed in time (time is measured in game ticks)
  std::priority_queue<DelayedActionQueueItem> delayedActions_;
};

}  // namespace griddly