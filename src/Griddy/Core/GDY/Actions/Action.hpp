#pragma once

#include <string>

#include "../Objects/Object.hpp"
#include "../Objects/GridLocation.hpp"

namespace griddy {

enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

class Action {
 public:
  Action(std::string actionTypeName, GridLocation sourceLocation, Direction direction);

  // The location of the unit to perform the action
  GridLocation getSourceLocation() const;

  // The location of the target of the action
  GridLocation getDestinationLocation() const;

  Direction getDirection() const;

  std::string getActionName() const;

  std::string getDescription() const;
  ~Action();

 protected:
  const GridLocation sourceLocation_;
  const std::string actionName_;
  const Direction direction_;
};
}  // namespace griddy