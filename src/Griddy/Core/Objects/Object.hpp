#pragma once

#include <memory>
#include <string>

#include "ObjectTypes.hpp"
#include "GridLocation.hpp"

namespace griddy {

class Action;

class Object {
 public:
  GridLocation getLocation();

  void setLocation(GridLocation location);

  virtual ObjectType getType() const = 0;

  // An action has been performed on this object by another object, such as a movement, harvest, attack etc
  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) = 0;

  // This object has performed an action
  virtual bool onPerformAction(std::shared_ptr<Action> action) = 0;

  // Can this object perform any action
  virtual bool canPerformAction(std::shared_ptr<Action> action);

  virtual ~Object() = 0;

 private:
  int x = -1;
  int y = -1;
};

}  // namespace griddy