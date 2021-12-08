#include "Action.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

namespace griddly {

Action::Action(std::shared_ptr<Grid> grid, std::string actionName, uint32_t playerId, uint32_t delay, std::unordered_map<std::string, int32_t> metaData)
    : actionName_(actionName),
      delay_(delay),
      playerId_(playerId),
      grid_(grid),
      metaData_(metaData) {
}

std::string Action::getDescription() const {
  if (!sourceObject_.expired()) {
    auto sourceLocation = getSourceLocation();
    auto destinationLocation = getDestinationLocation();
    return fmt::format("Action: {0} [{1}, {2}]->[{3}, {4}] [{5}, {6}] Delay: [{7}]",
                      actionName_,
                      sourceLocation.x, sourceLocation.y,
                      destinationLocation.x, destinationLocation.y,
                      vectorToDest_.x, vectorToDest_.y,
                      delay_);
  } else {
    return fmt::format("Action: source object expired, action will be ignored");
  }
}

void Action::init(glm::ivec2 sourceLocation, glm::ivec2 destinationLocation) {
  sourceLocation_ = sourceLocation;
  destinationLocation_ = destinationLocation;

  vectorToDest_ = destinationLocation - sourceLocation;

  actionMode_ = ActionMode::SRC_LOC_DST_LOC;
}

void Action::init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject) {
  sourceObject_ = sourceObject;
  destinationObject_ = destinationObject;

  vectorToDest_ = destObj()->getLocation() - sourceObj()->getLocation();

  actionMode_ = ActionMode::SRC_OBJ_DST_OBJ;
}

void Action::init(std::shared_ptr<Object> sourceObject, glm::ivec2 vectorToDest, glm::ivec2 orientationVector, bool relativeToSource) {
  sourceObject_ = sourceObject;

  spdlog::debug("Getting rotation matrix from source");
  auto rotationMatrix = sourceObj()->getObjectOrientation().getRotationMatrix();

  vectorToDest_ = relativeToSource ? vectorToDest * rotationMatrix : vectorToDest;
  orientationVector_ = relativeToSource ? orientationVector * rotationMatrix : orientationVector;

  spdlog::debug("SRC_OBJ_DST_VEC");
  actionMode_ = ActionMode::SRC_OBJ_DST_VEC;
}

std::shared_ptr<Object> Action::getSourceObject() const {
  if (sourceObj() != nullptr) {
    return sourceObj();
  } else {
    auto srcObject = grid()->getObject(sourceLocation_);
    if (srcObject != nullptr) {
      return srcObject;
    }

    spdlog::debug("getting default object");

    return grid()->getPlayerDefaultObject(playerId_);
  }
}

std::shared_ptr<Object> Action::getDestinationObject() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
    case ActionMode::SRC_OBJ_DST_LOC: {
      auto dstObject = grid()->getObject(destinationLocation_);
      if (dstObject != nullptr) {
        return dstObject;
      }
      return grid()->getPlayerDefaultObject(playerId_);
    }
    case ActionMode::SRC_OBJ_DST_OBJ:
      return destObj();
    case ActionMode::SRC_OBJ_DST_VEC: {
      if (!sourceObject_.expired()) {
        auto destinationLocation = (getSourceLocation() + vectorToDest_);
        auto dstObject = grid()->getObject(destinationLocation);
        if (dstObject != nullptr) {
          return dstObject;
        }
      }
      return grid()->getPlayerDefaultObject(playerId_);
    }
  }

  return nullptr;
}

glm::ivec2 Action::getSourceLocation() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
      return sourceLocation_;
    case ActionMode::SRC_OBJ_DST_LOC:
    case ActionMode::SRC_OBJ_DST_OBJ:
    case ActionMode::SRC_OBJ_DST_VEC:
      return sourceObj()->getLocation();
  }

  return {};
}

glm::ivec2 Action::getDestinationLocation() const {
  switch (actionMode_) {
    case ActionMode::SRC_LOC_DST_LOC:
    case ActionMode::SRC_OBJ_DST_LOC:
      return destinationLocation_;
    case ActionMode::SRC_OBJ_DST_OBJ:
      return destObj()->getLocation();
    case ActionMode::SRC_OBJ_DST_VEC:
      return sourceObj()->getLocation() + vectorToDest_;
  }

  return {};
}

glm::ivec2 Action::getVectorToDest() const {
  return vectorToDest_;
}

glm::ivec2 Action::getOrientationVector() const {
  return orientationVector_;
}

std::string Action::getActionName() const { return actionName_; }

uint32_t Action::getOriginatingPlayerId() const {
  return playerId_;
}

uint32_t Action::getDelay() const {
  return delay_;
}

int32_t Action::getMetaData(std::string variableName) const {
  return metaData_.at(variableName);
}

std::unordered_map<std::string, int32_t> Action::getMetaData() const {
  return metaData_;
}

std::shared_ptr<Object> Action::sourceObj() const {
  return sourceObject_.expired() ? nullptr : sourceObject_.lock();
}

std::shared_ptr<Object> Action::destObj() const {
  return destinationObject_.expired() ? nullptr : destinationObject_.lock();
}

std::shared_ptr<Grid> Action::grid() const {
  return grid_.lock();
}

}  // namespace griddly