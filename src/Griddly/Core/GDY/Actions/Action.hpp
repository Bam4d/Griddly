#pragma once

#include <string>

#include "../Objects/Object.hpp"
#include "Direction.hpp"
#include "../Objects/GridLocation.hpp"

namespace griddly {

class Action {
 public:
  Action(std::string actionName, GridLocation sourceLocation, uint32_t actionId, uint32_t delay=0);

  // The location of the unit to perform the action
  virtual GridLocation getSourceLocation() const;

  // The location of the target of the action
  virtual GridLocation getDestinationLocation(std::shared_ptr<Object> sourceObject) const;

  virtual Direction getDirection(std::shared_ptr<Object> sourceObject) const;

  virtual uint32_t getActionId() const;

  virtual std::string getActionName() const;

  virtual std::string getDescription() const;

  // Delay an action
  virtual uint32_t getDelay() const;

  ~Action();

 protected:
  const GridLocation sourceLocation_;
  const std::string actionName_;
  const uint32_t actionId_;
  const uint32_t delay_;
};
}  // namespace griddly