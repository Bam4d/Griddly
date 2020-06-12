#pragma once

#include <string>

#include "../Objects/Object.hpp"
#include "Direction.hpp"
#include "../Objects/GridLocation.hpp"

namespace griddly {

class Action {
 public:
  Action(std::string actionName, GridLocation sourceLocation, Direction direction);

  // The location of the unit to perform the action
  virtual GridLocation getSourceLocation() const;

  // The location of the target of the action
  virtual GridLocation getDestinationLocation() const;

  virtual Direction getDirection() const;

  virtual std::string getActionName() const;

  virtual std::string getDescription() const;
  ~Action();

 protected:
  const GridLocation sourceLocation_;
  const std::string actionName_;
  const Direction direction_;
};
}  // namespace griddly