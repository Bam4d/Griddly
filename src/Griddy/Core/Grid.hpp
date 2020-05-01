#pragma once

#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/GridLocation.hpp"
#include "LevelGenerators/LevelGenerator.hpp"

namespace griddy {

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  ~Grid();

  void init(uint32_t height, uint32_t width);

  void cloneState() const;
  std::vector<int> performActions(int playerId, std::vector<std::shared_ptr<Action>> actions);
  void update();

  bool updateLocation(std::shared_ptr<Object> object, GridLocation previousLocation, GridLocation newLocation);

  virtual std::unordered_set<GridLocation, GridLocation::Hash> getUpdatedLocations() const;

  uint32_t getCurrentScore(int playerId) const;
  uint32_t getResources(int playerId) const;

  uint32_t getWidth() const;
  uint32_t getHeight() const;

  uint32_t getTickCount() const;

  virtual void initObject(uint32_t playerId, GridLocation location, std::shared_ptr<Object> object);
  virtual bool removeObject(std::shared_ptr<Object> object);

  std::unordered_set<std::shared_ptr<Object>>& getObjects();

  virtual std::shared_ptr<Object> getObject(GridLocation location) const;

 private:
  uint32_t height_;
  uint32_t width_;

  uint32_t gameTick;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::unordered_set<GridLocation, GridLocation::Hash> updatedLocations_;

  std::set<std::string> availableObjects_;
  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, std::shared_ptr<Object>, GridLocation::Hash> occupiedLocations_;
};

}  // namespace griddy