#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Actions/Action.hpp"
#include "LevelGenerators/LevelGenerator.hpp"
#include "GDY/Objects/GridLocation.hpp"

namespace griddy {

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  void init(uint height, uint width);

  void cloneState() const;
  std::vector<int> performActions(int playerId, std::vector<std::shared_ptr<Action>> actions);
  void update();

  bool updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation);

  virtual std::unordered_set<GridLocation, GridLocation::Hash> getUpdatedLocations() const;

  uint getCurrentScore(int playerId) const;
  uint getResources(int playerId) const;

  uint getWidth() const;
  uint getHeight() const;

  uint getTickCount() const;

  virtual void initObject(uint playerId, GridLocation location, std::shared_ptr<Object> object);
  virtual bool removeObject(std::shared_ptr<Object> object);

  std::unordered_set<std::shared_ptr<Object>>& getObjects();

  virtual std::shared_ptr<Object> getObject(GridLocation location) const;

 private:
  uint height_;
  uint width_;

  uint gameTick;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::unordered_set<GridLocation, GridLocation::Hash> updatedLocations_;

  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, std::shared_ptr<Object>, GridLocation::Hash> occupiedLocations_;
};

}  // namespace griddy