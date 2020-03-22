#include "PushableWall.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"
#include "../Units/Pusher.hpp"
#include "../../Grid.hpp"

namespace griddy {

const ObjectType PushableWall::type = ObjectType::PUSHABLE_WALL;

std::string PushableWall::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, value={3}",
      x,
      y,
      "Resource",
      value);
}

ObjectType PushableWall::getObjectType() const { return type; }

bool PushableWall::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  auto actionType = action->getActionType();
  switch (actionType) {
    case MOVE:
      if (sourceObject->getObjectType() == PUSHER) {
        auto sourceLocation = sourceObject->getLocation();
        auto vector = GridLocation{x - sourceLocation.x, y - sourceLocation.y};
        auto pushLocation = GridLocation{x + vector.x, y + vector.y};

        // Can only be pushed into an empty space
        auto nextObject = grid_->getObject(pushLocation);
        if (nextObject == nullptr) {
          moveObject(pushLocation);
          return true;
        } else {
          return false;
        }
      }
      break;
    default:
      return false;
  }

}

PushableWall::PushableWall() {
}

PushableWall::~PushableWall() {}
}  // namespace griddy