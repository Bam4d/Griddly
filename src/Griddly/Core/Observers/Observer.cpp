#include "Observer.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(ObserverConfig observerConfig) {
  spdlog::debug("Initializing observer.");

  if (observerState_ != ObserverState::NONE) {
    throw std::runtime_error("Cannot initialize an already initialized Observer");
  }

  observerConfig_ = observerConfig;
  observerState_ = ObserverState::INITIALISED;
}

void Observer::reset() {
  spdlog::debug("Resetting observer.");
  if (observerState_ == ObserverState::NONE) {
    throw std::runtime_error("Observer not initialized");
  }
  resetShape();

  spdlog::debug("Observation Shape ({0}, {1}, {2})", observationShape_[0], observationShape_[1], observationShape_[2]);

  // if the observer is "READY", then it has already been initialized once, so keep it in the ready state, we're just resetting it.
  observerState_ = observerState_ == ObserverState::READY ? ObserverState::READY : ObserverState::RESET;
}

void Observer::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;
}

void Observer::print(std::shared_ptr<uint8_t> observation) {
}

std::vector<uint32_t> Observer::getShape() const {
  return observationShape_;
}

std::vector<uint32_t> Observer::getStrides() const {
  return observationStrides_;
}

glm::ivec2 Observer::getTileSize() const {
  return observerConfig_.tileSize;
}

void Observer::release() {
}

PartialObservableGrid Observer::getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation) const {
  PartialObservableGrid partiallyObservableGrid;

  switch (avatarOrientation) {
    case Direction::NONE:
    case Direction::UP:
      partiallyObservableGrid.left = avatarLocation.x - observerConfig_.gridXOffset - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - observerConfig_.gridXOffset + (gridWidth_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - observerConfig_.gridYOffset - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - observerConfig_.gridYOffset + (gridHeight_ - 1) / 2;
      break;
    case Direction::RIGHT:
      partiallyObservableGrid.left = avatarLocation.x + observerConfig_.gridYOffset - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + observerConfig_.gridYOffset + (gridHeight_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + observerConfig_.gridXOffset - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + observerConfig_.gridXOffset + (gridWidth_ - 1) / 2;
      break;
    case Direction::DOWN:
      partiallyObservableGrid.left = avatarLocation.x + observerConfig_.gridXOffset - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + observerConfig_.gridXOffset + (gridWidth_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + observerConfig_.gridYOffset - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + observerConfig_.gridYOffset + (gridHeight_ - 1) / 2;
      break;
    case Direction::LEFT:
      partiallyObservableGrid.left = avatarLocation.x - observerConfig_.gridYOffset - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - observerConfig_.gridYOffset + (gridHeight_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - observerConfig_.gridXOffset - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - observerConfig_.gridXOffset + (gridWidth_ - 1) / 2;
      break;
  }

  return partiallyObservableGrid;
}

Observer::~Observer() {
  spdlog::debug("Observer Destroyed");
}

}  // namespace griddly