#include <spdlog/spdlog.h>

#include "Observer.hpp"

#include <utility>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(std::move(grid)) {
}

void Observer::init(ObserverConfig& config) {
  spdlog::debug("Initializing observer.");

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

std::string Observer::getDefaultObserverName(ObserverType observerType) {
  switch (observerType) {
    case ObserverType::ASCII:
      return "ASCII";
    case ObserverType::VECTOR:
      return "VECTOR";
    case ObserverType::SPRITE_2D:
      return "SPRITE_2D";
    case ObserverType::BLOCK_2D:
      return "BLOCK_2D";
    case ObserverType::ISOMETRIC:
      return "ISOMETRIC";
    case ObserverType::NONE:
      return "NONE";
  }
}

}  // namespace griddly
