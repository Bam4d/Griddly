#include "Object.hpp"
#include "../../Grid.hpp"

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

int Object::onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return 0;
}

int Object::onActionDst(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
  return 0;
}

void Object::addActionSrcBehavour(std::string action, std::function<bool> behaviour) {

  srcBehavours_.insert({action, behaviour});

}

void Object::addActionDstBehavour(std::string action, std::function<bool> behaviour) {
  dstBehavours_.insert({action, behaviour});
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

Object::Object(std::string objectName, std::unordered_map<std::string, std::shared_ptr<uint>> parameters): objectName_(objectName) {
    parameters.insert({"_x", std::shared_ptr<uint>(x_)});
    parameters.insert({"_y", std::shared_ptr<uint>(y_)});

    parameters_ = parameters;
}

Object::~Object() {}


}  // namespace griddy