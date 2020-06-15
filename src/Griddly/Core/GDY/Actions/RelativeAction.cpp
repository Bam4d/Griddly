#include "RelativeAction.hpp"

namespace griddly {

RelativeAction::RelativeAction(std::string actionName, GridLocation sourceLocation, uint32_t actionId)
    : Action(actionName, sourceLocation, actionId) {
}

RelativeAction::~RelativeAction() {
}

GridLocation RelativeAction::getDestinationLocation(std::shared_ptr<Object> sourceObject) const {
  auto sourceObjectOrientation = sourceObject->getObjectOrientation();
  switch (actionId_) {
    default:
      return {
          sourceLocation_.x,
          sourceLocation_.y};
    // Up is the only input that we change the location ,
    // we move relative to the direction of the source object only
    case 2:  // maps to "up" or "w" on keyboard
      switch (sourceObjectOrientation) {
        case Direction::UP:
        case Direction::NONE:
          return {
              sourceLocation_.x,
              sourceLocation_.y - 1};
        case Direction::RIGHT:
          return {
              sourceLocation_.x + 1,
              sourceLocation_.y};
        case Direction::DOWN:
          return {
              sourceLocation_.x,
              sourceLocation_.y + 1};
        case Direction::LEFT:
          return {
              sourceLocation_.x - 1,
              sourceLocation_.y};
      }
  }
}

Direction RelativeAction::getDirection(std::shared_ptr<Object> sourceObject) const {
  auto sourceObjectOrientation = sourceObject->getObjectOrientation();

  switch (actionId_) {
    default:
      return sourceObjectOrientation;
    case 1:  // LEFT
      switch (sourceObjectOrientation) {
        case Direction::NONE:
        case Direction::UP:
          return Direction::LEFT;
        case Direction::RIGHT:
          return Direction::UP;
        case Direction::DOWN:
          return Direction::RIGHT;
        case Direction::LEFT:
          return Direction::DOWN;
      }
    case 3: // RIGHT
      switch (sourceObjectOrientation) {
        case Direction::NONE:
        case Direction::UP:
          return Direction::RIGHT;
        case Direction::RIGHT:
          return Direction::DOWN;
        case Direction::DOWN:
          return Direction::LEFT;
        case Direction::LEFT:
          return Direction::UP;
      }
  }
}
}  // namespace griddly