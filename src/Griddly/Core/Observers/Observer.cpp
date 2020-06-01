#include "Observer.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(ObserverConfig observerConfig) {

  if(observerConfig.gridWidth <= 0 || observerConfig.gridHeight <= 0) {
    throw std::invalid_argument("observer width and height must be greater than 0");
  }

  observerConfig_ = observerConfig;
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

PartialObservableGrid Observer::getAvatarObservableGrid(GridLocation avatarLocation, Direction avatarOrientation) const {
  PartialObservableGrid partiallyObservableGrid;
  switch (avatarOrientation) {
    case Direction::NONE:
    case Direction::UP:
      partiallyObservableGrid.left = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.right = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.bottom = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.top = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
      break;
    case Direction::RIGHT:
      partiallyObservableGrid.left = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.right = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.bottom = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.top = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
      break;
    case Direction::DOWN:
      partiallyObservableGrid.left = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.right = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.bottom = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.top = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
      break;
    case Direction::LEFT:
      partiallyObservableGrid.left = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.right = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
      partiallyObservableGrid.bottom = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
      partiallyObservableGrid.top = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
      break;
  }

  return partiallyObservableGrid;
}

Observer::~Observer() {
}

}  // namespace griddly