#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Actions/Action.hpp"
#include "LevelGenerators/LevelGenerator.hpp"
#include "Objects/GridLocation.hpp"

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

  int getCurrentScore(int playerId) const;
  int getResources(int playerId) const;

  uint getWidth() const;
  uint getHeight() const;

  int getTickCount() const;

  virtual void initObject(GridLocation location, std::shared_ptr<Object> object);
  virtual bool removeObject(std::shared_ptr<Object> object);

  std::unordered_set<std::shared_ptr<Object>>& getObjects();

  virtual std::shared_ptr<Object> getObject(GridLocation location) const;

 private:
  uint height_;
  uint width_;

  int gameTick;

  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, std::shared_ptr<Object>, GridLocation::Hash> occupiedLocations_;
};

}  // namespace griddy