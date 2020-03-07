#include "Object.hpp"

namespace griddy {

class Action;

const GridLocation Object::getLocation() const {
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

int Object::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return 0;
}

Object::~Object() {}


}  // namespace griddy