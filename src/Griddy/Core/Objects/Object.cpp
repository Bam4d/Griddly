#include "Object.hpp"

namespace griddy {

class Action;

GridLocation Object::getLocation() {
  GridLocation location(x, y);
  return location;
};

void Object::setLocation(GridLocation location) {
  this->x = location.x;
  this->y = location.y;
}

bool Object::canPerformAction(std::shared_ptr<Action> action) {
  return false;
}

bool Object::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return false;
}

Object::~Object() {}


}  // namespace griddy