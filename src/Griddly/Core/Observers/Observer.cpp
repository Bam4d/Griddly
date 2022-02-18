#include "Observer.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(int32_t gridXOffset, int32_t gridYOffset) {
  spdlog::debug("Initializing observer.");

  if (observerState_ != ObserverState::NONE) {
    throw std::runtime_error("Cannot initialize an already initialized Observer");
  }

  observerState_ = ObserverState::INITIALISED;

  gridXOffset_ = gridXOffset;
  gridYOffset_ = gridYOffset;
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

PartialObservableGrid Observer::getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation) const {
  PartialObservableGrid partiallyObservableGrid;

  switch (avatarOrientation) {
    case Direction::NONE:
    case Direction::UP:
      partiallyObservableGrid.left = avatarLocation.x - gridXOffset_ - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - gridXOffset_ + (gridWidth_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - gridYOffset_ - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - gridYOffset_ + (gridHeight_ - 1) / 2;
      break;
    case Direction::RIGHT:
      partiallyObservableGrid.left = avatarLocation.x + gridYOffset_ - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + gridYOffset_ + (gridHeight_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + gridXOffset_ - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + gridXOffset_ + (gridWidth_ - 1) / 2;
      break;
    case Direction::DOWN:
      partiallyObservableGrid.left = avatarLocation.x + gridXOffset_ - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x + gridXOffset_ + (gridWidth_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y + gridYOffset_ - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y + gridYOffset_ + (gridHeight_ - 1) / 2;
      break;
    case Direction::LEFT:
      partiallyObservableGrid.left = avatarLocation.x - gridYOffset_ - (gridHeight_ - 1) / 2;
      partiallyObservableGrid.right = avatarLocation.x - gridYOffset_ + (gridHeight_ - 1) / 2;
      partiallyObservableGrid.bottom = avatarLocation.y - gridXOffset_ - (gridWidth_ - 1) / 2;
      partiallyObservableGrid.top = avatarLocation.y - gridXOffset_ + (gridWidth_ - 1) / 2;
      break;
  }

  return partiallyObservableGrid;
}

}  // namespace griddly