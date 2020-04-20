#include "Puncher.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <memory>
#include "../../Actions/Action.hpp"
#include "../../Actions/Move.hpp"
#include "../../Grid.hpp"

namespace griddy {

class Object;

const ObjectType Puncher::type = ObjectType::PUNCHER;

ObjectType Puncher::getObjectType() const { return type; }

std::string Puncher::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, health={3}",
      x_,
      y_,
      "Puncher",
      health_);
};

bool Puncher::canPerformAction(std::shared_ptr<griddy::Action> action) {
  return action->getActionType() == ActionType::PUNCH || action->getActionType() == ActionType::MOVE;
}

int Puncher::onPerformAction(std::shared_ptr<griddy::Object> destinationObject, std::shared_ptr<griddy::Action> action) {
  auto actionType = action->getActionType();

  switch (actionType) {
    case PUNCH:
      if (destinationObject != nullptr && (destinationObject->getObjectType() == HARVESTER || destinationObject->getObjectType() == PUNCHER || destinationObject->getObjectType() == PUSHER)) {
        return 1;
      }
      break;
    case MOVE:
      if (destinationObject == nullptr) {
        moveObject(action->getDestinationLocation());
        return 0;
      }
      break;
    default:
      return 0;
  }

  return 0;
}

bool Puncher::onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
  auto actionType = action->getActionType();

  switch (actionType) {
    case PUNCH:
      health_ -= 1;

      if (health_ == 0) {
        removeObject();
      }
      return true;
      break;
    case MOVE:
      if (sourceObject->getObjectType() == PUSHER) {
        auto sourceLocation = sourceObject->getLocation();
        auto vector = GridLocation{x_ - sourceLocation.x, y_ - sourceLocation.y};
        auto pushLocation = GridLocation{x_ + vector.x, y_ + vector.y};

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

Puncher::Puncher() : Unit(3) {}

Puncher::~Puncher() {}
}  // namespace griddy