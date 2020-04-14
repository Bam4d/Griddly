#include "Object.hpp"
#include "../Grid.hpp"

namespace griddy {

class Action;

GridLocation Object::getLocation() const {
  GridLocation location(x_, y_);
  return location;
};

void Object::init(uint playerId, GridLocation location, std::shared_ptr<Grid> grid) {
  x_ = location.x;
  y_ = location.y;

  grid_ = grid;

  playerId_ = playerId;
}

bool Object::canPerformAction(std::shared_ptr<Action> action) {
  return false;
}

int Object::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return 0;
}

uint Object::getPlayerId() const {
  return playerId_;
}

void Object::moveObject(GridLocation newLocation) {
  grid_->updateLocation(shared_from_this(), {x_, y_}, newLocation);
  x_ = newLocation.x;
  y_ = newLocation.y;
}

void Object::removeObject() {
  grid_->removeObject(shared_from_this());
}

Object::~Object() {}


}  // namespace griddy