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
  return action->getActionType() == ActionType::GATHER;
}

bool Harvester::onPerformAction(std::shared_ptr<griddy::Object> destinationObject, std::shared_ptr<griddy::Action> action) {
  if (action->getActionType() == ActionType::GATHER) {
    if (destinationObject->getObjectType() == ObjectType::RESOURCE) {
      resources += 1;
    }
  }
}

Harvester::Harvester(int playerId) : Unit(playerId, 10) {}

Harvester::~Harvester() {}
}  // namespace griddy