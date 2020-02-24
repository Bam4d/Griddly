#pragma once

#include <string>
#include "../Objects/GridLocation.hpp"
#include "../Objects/Object.hpp"
#include "ActionTypes.hpp"

namespace griddy {

enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

class Action {
 public:
  Action(GridLocation targetLocation, std::string actionTypeName, ActionType actionType);

  // The location of the unit to perform the action
  GridLocation getSourceLocation() const;

  // The location of the target of the action
  virtual GridLocation getDestinationLocation() const;
  std::string getActionTypeName() const;
  ActionType getActionType() const;

  virtual std::string getDescription() const = 0;
  virtual ~Action() = 0;

 protected:
  const GridLocation targetLocation_;
  const std::string actionTypeName_;
  const ActionType actionType_;
};
}  // namespace griddy