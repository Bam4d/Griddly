#include "Observer.hpp"

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(ObserverConfig observerConfig) {
}

void Observer::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;
}

PartialObservationRect Observer::getPartialObservationRect() const {
  auto maxHeight = grid->getHeight();
  auto maxWidth = grid->getWidth();
  auto avatarLocation = avatarObject_->getLocation();
  auto maxX = avatarLocation.x + observerConfig_.xOffset + observerConfig_.width / 2;
  auto minX = avatarLocation.x + observerConfig_.xOffset - observerConfig_.width / 2;
  auto maxY = avatarLocation.y + observerConfig_.yOffset + observerConfig_.height / 2;
  auto minY = avatarLocation.y + observerConfig_.yOffset - observerConfig_.height / 2;

  auto top = maxY > maxHeight ? maxHeight : maxY;
  auto bottom = minY < 0 ? 0 : minY;
  auto left = minX < 0 ? 0 : minX;
  auto right = maxX > maxWidth ? maxWidth : maxX;

  return {top, bottom, left, right};
}

void Observer::print(std::shared_ptr<uint8_t> observation) {
}

Observer::~Observer() {
}

}  // namespace griddly