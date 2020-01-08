#include "Object.hpp"

namespace griddy {

GridLocation Object::getLocation() {
  GridLocation location(x, y);
  return location;
};

void Object::setLocation(GridLocation location) {
  this->x = location.x;
  this->y = location.y;
}

Object::~Object() {}

}  // namespace griddy