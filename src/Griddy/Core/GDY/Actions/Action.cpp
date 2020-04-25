#include "Action.hpp"

namespace griddy {

namespace gdy {

Action::Action(std::string actionName, GridLocation sourceLocation, Direction direction) : sourceLocation_(sourceLocation),
                                                                                          actionName_(actionName),
                                                                                          direction_(direction) {
}

Action::~Action() {}

std::string Action::getActionName() const { return actionName_; }

GridLocation Action::getSourceLocation() const { return sourceLocation_; }

// By Default the destination location is the same as the target
GridLocation Action::getDestinationLocation() const { return sourceLocation_; }

Direction Action::getDirection() const {
    return direction_;
}

}  // namespace gdy
}  // namespace griddy