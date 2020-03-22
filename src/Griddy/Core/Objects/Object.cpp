#include "Object.hpp"
#include "../Grid.hpp"

namespace griddy {

class Action;

const GridLocation Object::getLocation() const {
  GridLocation location(x, y);
  return location;
};

void Object::init(GridLocation location, std::shared_ptr<Grid> grid) {
  x = location.x;
  y = location.y;

  grid_ = grid;

}

bool Object::canPerformAction(std::shared_ptr<Action> action) {
  return false;
}

int Object::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return 0;
}

void Object::moveObject(GridLocation newLocation) {
  grid_->updateLocation(shared_from_this(), {x, y}, newLocation);
  x = newLocation.x;
  y = newLocation.y;
}

Object::~Object() {}


}  // namespace griddy