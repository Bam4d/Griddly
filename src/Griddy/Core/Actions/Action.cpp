#include "Action.hpp"

namespace griddy {

Action::Action(GridLocation targetLocation, std::string actionTypeName, ActionType actionType) : targetLocation_(targetLocation),
                                                                          actionTypeName_(actionTypeName),
                                                                          actionType_(actionType) {
}

Action::~Action() {}

std::string Action::getDescription() const {
  return std::string("Action");
}

std::string Action::getActionTypeName() const { return actionTypeName_; }

GridLocation Action::getSourceLocation() const { return targetLocation_; }

// By Default the destination location is the same as the target
GridLocation Action::getDestinationLocation() const { return targetLocation_; }

unsigned long Action::getActionType() const { return actionType_; }

}  // namespace griddy