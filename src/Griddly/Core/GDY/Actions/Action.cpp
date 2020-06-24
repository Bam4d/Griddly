#include "Action.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

namespace griddly {

Action::Action(std::string actionName, GridLocation sourceLocation, uint32_t actionId, uint32_t delay)
    : sourceLocation_(sourceLocation),
      actionName_(actionName),
      actionId_(actionId),
      delay_(delay) {
}

Action::~Action() {}

std::string Action::getDescription() const {
  return fmt::format("Action: {0} [{1}] [{2}, {3}]",
                     actionName_,
                     actionId_,
                     sourceLocation_.x,
                     sourceLocation_.y);
}

std::string Action::getActionName() const { return actionName_; }

GridLocation Action::getSourceLocation() const { return sourceLocation_; }

// By Default the destination location is the same as the target
GridLocation Action::getDestinationLocation(std::shared_ptr<Object> sourceObject) const {
  switch (actionId_) {
    case 0:
      return {
          sourceLocation_.x,
          sourceLocation_.y};
    case 1:  // LEFT
      return {
          sourceLocation_.x - 1,
          sourceLocation_.y};
    case 2:  // UP
      return {
          sourceLocation_.x,
          sourceLocation_.y - 1};
    case 3:  //RIGHT
      return {
          sourceLocation_.x + 1,
          sourceLocation_.y};
    case 4:  //DOWN
      return {
          sourceLocation_.x,
          sourceLocation_.y + 1};
  }
}

Direction Action::getDirection(std::shared_ptr<Object> sourceObject) const {
  switch (actionId_) {
    case 0:
      return Direction::NONE;
    case 1:
      return Direction::LEFT;
    case 2:
      return Direction::UP;
    case 3:
      return Direction::RIGHT;
    case 4:
      return Direction::DOWN;
  }
}

uint32_t Action::getActionId() const {
  return actionId_;
}

uint32_t Action::getDelay() const {
  return delay_;
}

}  // namespace griddly