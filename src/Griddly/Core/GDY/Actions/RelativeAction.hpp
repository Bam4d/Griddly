#pragma once

#include <string>

#include "../Objects/Object.hpp"
#include "Direction.hpp"
#include "../Objects/GridLocation.hpp"
#include "Action.hpp"

namespace griddly {

/**
 * Action that is relative to the rotation of the source object
 */
class RelativeAction : public Action {
 public:
  RelativeAction(std::string actionName, GridLocation sourceLocation, uint32_t actionId);


  // The location of the target of the action
  virtual GridLocation getDestinationLocation(std::shared_ptr<Object> sourceObject) const override;

  virtual Direction getDirection(std::shared_ptr<Object> sourceObject) const override;

  ~RelativeAction();
};
}  // namespace griddly