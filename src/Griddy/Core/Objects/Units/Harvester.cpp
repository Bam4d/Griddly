#include "Harvester.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"
#include "../../Grid.hpp"

namespace griddy {

class Object;

const ObjectType Harvester::type = ObjectType::HARVESTER;

ObjectType Harvester::getObjectType() const { return type; }

std::string Harvester::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, health={3}",
      x,
      y,
      "Harvester",
      health_);
};

bool Harvester::canPerformAction(std::shared_ptr<griddy::Action> action) {
  return action->getActionType() == ActionType::GATHER || action->getActionType() == ActionType::MOVE;
}

int Harvester::onPerformAction(std::shared_ptr<griddy::Object> destinationObject, std::shared_ptr<griddy::Action> action) {
  auto actionType = action->getActionType();

  switch (actionType) {
    case GATHER:
      if (destinationObject != nullptr && destinationObject->getObjectType() == ObjectType::MINERALS) {
        if (resources_ < maxResources_) {
          resources_ += 1;
          return 1;
        }
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

bool Harvester::onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
  auto actionType = action->getActionType();

  switch (actionType) {
    case PUNCH:
      health_ -= 1;
      return true;
      break;
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

int Harvester::getResources() const {
  return resources_;
}

Harvester::Harvester(int playerId) : Unit(playerId, 10) {}

Harvester::~Harvester() {}
}  // namespace griddy