#include "Action.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

namespace griddly {

Action::Action(std::shared_ptr<Grid> grid, std::string actionName, uint32_t delay)
    : actionName_(actionName),
      delay_(delay),
      grid_(grid) {
}

Action::~Action() {}

std::string Action::getDescription() const {
  return fmt::format("Action: {0} Delay: {4}",
                     actionName_,
                     delay_);
}

void Action::init(glm::ivec2 sourceLocation, glm::ivec2 destinationLocation) {
  sourceLocation_ = sourceLocation;
  destinationLocation_ = destinationLocation;

  actionMode_ = ActionMode::SRC_LOC_DST_LOC;
}

void Action::init(std::shared_ptr<Object> sourceObject, glm::ivec2 destinationLocation) {
  sourceObject_ = sourceObject_;
  destinationLocation_ = destinationLocation;

  actionMode_ = ActionMode::SRC_OBJ_DST_LOC;
}

void Action::init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject) {
  sourceObject_ = sourceObject_;
  destinationObject_ = destinationObject;

  actionMode_ = ActionMode::SRC_OBJ_DST_OBJ;
}

void Action::init(std::shared_ptr<Object> sourceObject, glm::ivec2 vector, bool relativeToSource) {
  sourceObject_ = sourceObject;

  auto orientedVector = relativeToSource ? sourceObject_->getObjectOrientation().getRelativeUnitVector(vector) : vector;

  destinationLocation_.x = sourceObject->getLocation().x + orientedVector.x;
  destinationLocation_.y = sourceObject->getLocation().y + orientedVector.y;

  actionMode_ = ActionMode::SRC_OBJ_DST_VEC;
}

void Action::init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject, glm::ivec2 vector, bool relativeToSource) {
  sourceObject_ = sourceObject;

  auto orientedVector = relativeToSource ? sourceObject_->getObjectOrientation().getRelativeUnitVector(vector) : vector;

  destinationLocation_.x = sourceObject->getLocation().x + orientedVector.x;
  destinationLocation_.y = sourceObject->getLocation().y + orientedVector.y;

  destinationObject_ = destinationObject;

  actionMode_ = ActionMode::SRC_OBJ_DST_LOC;
}

std::shared_ptr<Object> Action::getSourceObject() const {
  if (sourceObject_ != nullptr) {
    return sourceObject_;
  } else {
    return grid_->getObjectsAt(sourceLocation_)[0];
  }
}

std::shared_ptr<Object> Action::getDestinationObject() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
    case ActionMode::SRC_OBJ_DST_LOC:
      return grid_->getObjectsAt(destinationLocation_)[0];
    case ActionMode::SRC_OBJ_DST_OBJ:
      return destinationObject_;
    case ActionMode::SRC_OBJ_DST_VEC:
      return grid_->getObjectsAt(sourceLocation_ + vector_)[0];
  }
}

glm::ivec2 Action::getSourceLocation() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
      return sourceLocation_;
    case ActionMode::SRC_OBJ_DST_LOC:
    case ActionMode::SRC_OBJ_DST_OBJ:
    case ActionMode::SRC_OBJ_DST_VEC:
      return sourceObject_->getLocation();
  }
}

glm::ivec2 Action::getDestinationLocation() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
    case ActionMode::SRC_OBJ_DST_LOC:
      return destinationLocation_;
    case ActionMode::SRC_OBJ_DST_OBJ:
      return destinationObject_->getLocation();
    case ActionMode::SRC_OBJ_DST_VEC:
      return sourceLocation_ + vector_;
  }
}

glm::ivec2 Action::getVector() const {
  return vector_;
}

std::string Action::getActionName() const { return actionName_; }

// By Default the destination location is the same as the target
// glm::ivec2 Action::getDestinationLocation(std::shared_ptr<Object> sourceObject) const {
//   switch (actionId_) {
//     case 0:
//       return {
//           sourceLocation_.x,
//           sourceLocation_.y};
//     case 1:  // LEFT
//       return {
//           sourceLocation_.x - 1,
//           sourceLocation_.y};
//     case 2:  // UP
//       return {
//           sourceLocation_.x,
//           sourceLocation_.y - 1};
//     case 3:  //RIGHT
//       return {
//           sourceLocation_.x + 1,
//           sourceLocation_.y};
//     case 4:  //DOWN
//       return {
//           sourceLocation_.x,
//           sourceLocation_.y + 1};
//   }
// }

// Direction Action::getDirection(std::shared_ptr<Object> sourceObject) const {
//   switch (actionId_) {
//     case 0:
//       return Direction::NONE;
//     case 1:
//       return Direction::LEFT;
//     case 2:
//       return Direction::UP;
//     case 3:
//       return Direction::RIGHT;
//     case 4:
//       return Direction::DOWN;
//   }
// }

uint32_t Action::getDelay() const {
  return delay_;
}

}  // namespace griddly