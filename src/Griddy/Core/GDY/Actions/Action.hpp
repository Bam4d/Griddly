#pragma once

#include <string>

#include "../Objects/Object.hpp"
#include "../Objects/GridLocation.hpp"

namespace griddy {

enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NONE
};

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
}  // namespace griddy