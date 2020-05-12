#pragma once

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/GridLocation.hpp"
#include "LevelGenerators/LevelGenerator.hpp"

#define TileObjects std::map<uint32_t, std::shared_ptr<Object>>

namespace griddy {


class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  virtual void init(uint32_t height, uint32_t width);

  virtual std::vector<int> performActions(int playerId, std::vector<std::shared_ptr<Action>> actions);
  virtual void update();

  virtual bool updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation);

  virtual std::unordered_set<GridLocation, GridLocation::Hash> getUpdatedLocations() const;

  virtual uint32_t getWidth() const;
  virtual uint32_t getHeight() const;

  virtual uint32_t getTickCount() const;

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

  virtual std::unordered_map<uint32_t, std::shared_ptr<int32_t>> getObjectCounter(std::string objectName) const;

 private:
  uint32_t height_;
  uint32_t width_;

  uint32_t gameTick;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::unordered_set<GridLocation, GridLocation::Hash> updatedLocations_;

  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, TileObjects, GridLocation::Hash> occupiedLocations_;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> objectCounters_;
};

}  // namespace griddy