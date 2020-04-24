#pragma once

#include <string>

#include "../Objects/Object.hpp"

namespace griddy {

namespace gdy {

enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

class Action {
 public:
  Action(GridLocation sourceLocation, std::string actionTypeName, uint actionType);

  // The location of the unit to perform the action
  GridLocation getSourceLocation() const;

  // The location of the target of the action
  virtual GridLocation getDestinationLocation() const;
  std::string getActionTypeName() const;
  uint getActionType() const;

  virtual std::string getDescription() const = 0;
  virtual ~Action() = 0;

 protected:
  const GridLocation sourceLocation_;
  const std::string actionTypeName_;
  const uint actionType_;
};
}  // namespace gdy
}  // namespace griddy