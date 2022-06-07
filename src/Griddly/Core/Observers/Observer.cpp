#include "Observer.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(std::move(grid)) {
}

void Observer::init(ObserverConfig& config) {
  if (observerState_ != ObserverState::NONE) {
    throw std::runtime_error("Cannot initialize an already initialized Observer");
  }

  observerState_ = ObserverState::INITIALISED;

  config_ = config;
}

void Observer::reset() {
  spdlog::debug("Resetting observer.");
  if (observerState_ == ObserverState::NONE) {
    throw std::runtime_error("Observer not initialized");
  }
  resetShape();

  doTrackAvatar_ = avatarObject_ != nullptr && config_.trackAvatar;

  spdlog::debug("Tracking avatar: {0}", doTrackAvatar_ ? "Yes":"No");

  // if the observer is "READY", then it has already been initialized once, so keep it in the ready state, we're just resetting it.
  observerState_ = observerState_ == ObserverState::READY ? ObserverState::READY : ObserverState::RESET;
}

void Observer::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;
}

bool Observer::trackAvatar() const {
  return config_.trackAvatar;
}

void Observer::release() {
}

PartialObservableGrid Observer::getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation) const {
  PartialObservableGrid partiallyObservableGrid;

  switch (avatarOrientation) {
    case Direction::NONE:
    case Direction::UP:
      partiallyObservableGrid.left = avatarLocation.x - config_.gridXOffset - (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - config_.gridXOffset + (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - config_.gridYOffset - (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - config_.gridYOffset + (config_.overrideGridHeight - 1) / 2;
      break;
    case Direction::RIGHT:
      partiallyObservableGrid.left = avatarLocation.x + config_.gridYOffset - (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + config_.gridYOffset + (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + config_.gridXOffset - (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + config_.gridXOffset + (config_.overrideGridWidth - 1) / 2;
      break;
    case Direction::DOWN:
      partiallyObservableGrid.left = avatarLocation.x + config_.gridXOffset - (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + config_.gridXOffset + (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + config_.gridYOffset - (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + config_.gridYOffset + (config_.overrideGridHeight - 1) / 2;
      break;
    case Direction::LEFT:
      partiallyObservableGrid.left = avatarLocation.x - config_.gridYOffset - (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - config_.gridYOffset + (config_.overrideGridHeight - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - config_.gridXOffset - (config_.overrideGridWidth - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - config_.gridXOffset + (config_.overrideGridWidth - 1) / 2;
      break;
  }

  return partiallyObservableGrid;
}

PartialObservableGrid Observer::getObservableGrid() const {
  PartialObservableGrid observableGrid;

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    if (config_.rotateWithAvatar) {
      observableGrid = getAvatarObservableGrid(avatarLocation, avatarObject_->getObjectOrientation().getDirection());
    } else {
      observableGrid = getAvatarObservableGrid(avatarLocation);
    }
  } else {
    observableGrid = {
        static_cast<int32_t>(gridHeight_) - static_cast<int32_t>(config_.gridYOffset) - 1,
        -config_.gridYOffset,
        -config_.gridXOffset,
        static_cast<int32_t>(gridWidth_) + static_cast<int32_t>(config_.gridXOffset) - 1};
  }

  observableGrid.left = std::max(0, observableGrid.left);
  observableGrid.right = std::max(0, observableGrid.right);
  observableGrid.bottom = std::max(0, observableGrid.bottom);
  observableGrid.top = std::max(0, observableGrid.top);

  return observableGrid;
}

uint32_t Observer::getEgocentricPlayerId(uint32_t objectPlayerId) const {
  // if we are including the player ID, we always set player = 1 from the perspective of the agent being controlled.
  // e.g if this is observer is owned by player 3 then objects owned by player 3 will be rendered as "player 1".
  // This is so multi-agent games always see the agents they are controlling from first person perspective
  if (objectPlayerId == 0 || config_.playerId == 0) {
    return objectPlayerId;
  } else if (objectPlayerId < config_.playerId) {
    return objectPlayerId + 1;
  } else if (objectPlayerId == config_.playerId) {
    return 1;
  } else {
    return objectPlayerId;
  }
}

std::string Observer::getDefaultObserverName(ObserverType observerType) {
  switch (observerType) {
    case ObserverType::ASCII:
      return "ASCII";
    case ObserverType::VECTOR:
      return "VECTOR";
    case ObserverType::ENTITY:
      return "ENTITY";
    case ObserverType::NONE:
      return "NONE";
#ifndef WASM
    case ObserverType::SPRITE_2D:
      return "SPRITE_2D";
    case ObserverType::BLOCK_2D:
      return "BLOCK_2D";
    case ObserverType::ISOMETRIC:
      return "ISOMETRIC";
#endif
    
  }
}

}  // namespace griddly
