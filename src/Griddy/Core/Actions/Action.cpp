#include "Action.hpp"

namespace griddy {

Action::Action(GridLocation targetLocation, std::string actionTypeName)
    : targetLocation_(targetLocation), actionTypeName_(actionTypeName) {}

Action::~Action() {}

std::string Action::log() const {
  return std::string("Action");
}

std::string Action::getActionTypeName() const { return actionTypeName_; }

GridLocation Action::getTargetLocation() const { return targetLocation_; }

}  // namespace griddy