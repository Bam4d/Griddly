#include "Harvester.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"

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

  switch(actionType) {
    case GATHER:
      if (destinationObject != nullptr && destinationObject->getObjectType() == ObjectType::RESOURCE) {
        resources += 1;
        return 1;
      }
    break;
    case MOVE:
      if (destinationObject == nullptr) {
        auto newLocation = action->getDestinationLocation();
        x = newLocation.x;
        y = newLocation.y;
        return true;
      }
    break;
    default:
      return false;
  }

}

Harvester::Harvester(int playerId) : Unit(playerId, 10) {}

Harvester::~Harvester() {}
}  // namespace griddy