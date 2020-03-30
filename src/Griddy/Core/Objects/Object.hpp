#pragma once

#include <memory>
#include <string>

#include "ObjectTypes.hpp"
#include "GridLocation.hpp"

namespace griddy {

class Action;
class Grid;

class Object : public std::enable_shared_from_this<Object>{
 public:
  virtual GridLocation getLocation() const;

  void init(GridLocation location, std::shared_ptr<Grid> grid_);

  virtual ObjectType getObjectType() const = 0;

  virtual std::string getDescription() const = 0;

  // An action has been performed on this object by another object, such as a movement, harvest, attack etc
  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) = 0;

  // This object has performed an action
  virtual int onPerformAction(std::shared_ptr<griddy::Object> destinationObject, std::shared_ptr<Action> action);

  // Can this object perform any action
  virtual bool canPerformAction(std::shared_ptr<Action> action);

  virtual ~Object() = 0;

 protected:
  int x = -1;
  int y = -1;

  std::shared_ptr<Grid> grid_;

  virtual void moveObject(GridLocation newLocation);

  virtual void removeObject();
};

}  // namespace griddy