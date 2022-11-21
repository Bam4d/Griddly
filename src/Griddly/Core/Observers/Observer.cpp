#include "Observer.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid, ObserverConfig& config) : grid_(std::move(grid)), config_(std::move(config)) {
}

void Observer::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  if (observerState_ != ObserverState::NONE) {
    throw std::runtime_error("Cannot initialize an already initialized Observer");
  }

  observerState_ = ObserverState::INITIALISED;

  playerObservers_ = playerObservers;
}

void Observer::reset(std::shared_ptr<Object> avatarObject) {
  spdlog::debug("Resetting observer.");
  if (observerState_ == ObserverState::NONE) {
    throw std::runtime_error("Observer not initialized");
  }
  resetShape();

  avatarObject_ = avatarObject;

  doTrackAvatar_ = avatarObject_ != nullptr && config_.trackAvatar;

  spdlog::debug("Tracking avatar: {0}", doTrackAvatar_ ? "Yes":"No");

  // if the observer is "READY", then it has already been initialized once, so keep it in the ready state, we're just resetting it.
  observerState_ = observerState_ == ObserverState::READY ? ObserverState::READY : ObserverState::RESET;
}

bool Observer::trackAvatar() const {
  return config_.trackAvatar;
}

void Observer::release() {
}

PartialObservableGrid Observer::getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation) const {
  PartialObservableGrid partiallyObservableGrid;

  spdlog::debug("Avatar Location: ({0},{1})", avatarLocation.x, avatarLocation.y);

  spdlog::debug("Grid Offset: ({0},{1})", config_.gridXOffset, config_.gridYOffset);
  spdlog::debug("Override Grid Width: ({0},{1})", config_.overrideGridWidth, config_.overrideGridHeight);

  auto width = config_.overrideGridWidth > 0 ? config_.overrideGridWidth : gridWidth_;
  auto height = config_.overrideGridHeight > 0 ? config_.overrideGridHeight : gridHeight_;

  switch (avatarOrientation) {
    case Direction::NONE:
    case Direction::UP:
      partiallyObservableGrid.left = avatarLocation.x - config_.gridXOffset - (width - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - config_.gridXOffset + (width - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - config_.gridYOffset - (height - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - config_.gridYOffset + (height - 1) / 2;
      break;
    case Direction::RIGHT:
      partiallyObservableGrid.left = avatarLocation.x + config_.gridYOffset - (height - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + config_.gridYOffset + (height - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + config_.gridXOffset - (width - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + config_.gridXOffset + (width - 1) / 2;
      break;
    case Direction::DOWN:
      partiallyObservableGrid.left = avatarLocation.x + config_.gridXOffset - (width - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + config_.gridXOffset + (width - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + config_.gridYOffset - (height - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + config_.gridYOffset + (height - 1) / 2;
      break;
    case Direction::LEFT:
      partiallyObservableGrid.left = avatarLocation.x - config_.gridYOffset - (height - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - config_.gridYOffset + (height - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - config_.gridXOffset - (width - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - config_.gridXOffset + (width - 1) / 2;
      break;
  }

  spdlog::debug("Partially Observable grid: t:{0}, b:{1}, l:{2}, r:{3}", partiallyObservableGrid.top, partiallyObservableGrid.bottom, partiallyObservableGrid.left, partiallyObservableGrid.right);

  return partiallyObservableGrid;
}

PartialObservableGrid Observer::getObservableGrid() const {
  PartialObservableGrid observableGrid;

  if (doTrackAvatar_) {
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

  spdlog::debug("Observable grid: t:{0}, b:{1}, l:{2}, r:{3}", observableGrid.top, observableGrid.bottom, observableGrid.left, observableGrid.right);

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
