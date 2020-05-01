#include "Action.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

namespace griddy {

Action::Action(std::string actionName, GridLocation sourceLocation, Direction direction) : sourceLocation_(sourceLocation),
                                                                                          actionName_(actionName),
                                                                                          direction_(direction) {
}

Action::~Action() {}

std::string Action::getDescription() const {
    return fmt::format("Action: {0} [{1}, {2}]->[{3}, {4}]",
    actionName_, 
    sourceLocation_.x, 
    sourceLocation_.y,
    getDestinationLocation().x,
    getDestinationLocation().y);
}

std::string Action::getActionName() const { return actionName_; }

GridLocation Action::getSourceLocation() const { return sourceLocation_; }

// By Default the destination location is the same as the target
GridLocation Action::getDestinationLocation() const {
  switch (direction_) {
    case UP:
      return {
          sourceLocation_.x,
          sourceLocation_.y + 1};
    case RIGHT:
      return {
          sourceLocation_.x + 1,
          sourceLocation_.y};
    case DOWN:
      return {
          sourceLocation_.x,
          sourceLocation_.y - 1};
    case LEFT:
      return {
          sourceLocation_.x - 1,
          sourceLocation_.y};
  }
}

Direction Action::getDirection() const {
    return direction_;
}

}  // namespace griddy