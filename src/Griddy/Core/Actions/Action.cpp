#include "Action.hpp"

namespace griddy {

Action::Action(GridLocation sourceLocation, std::string actionTypeName, ActionType actionType) : sourceLocation_(sourceLocation),
                                                                          actionTypeName_(actionTypeName),
                                                                          actionType_(actionType) {
}

Action::~Action() {}

std::string Action::getActionTypeName() const { return actionTypeName_; }

GridLocation Action::getSourceLocation() const { return sourceLocation_; }

// By Default the destination location is the same as the target
GridLocation Action::getDestinationLocation() const { return sourceLocation_; }

ActionType Action::getActionType() const { return actionType_; }

}  // namespace griddy