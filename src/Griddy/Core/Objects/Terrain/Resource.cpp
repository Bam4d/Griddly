#include "Resource.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"
#include "../Units/Harvester.hpp"

namespace griddy {

const ObjectType Resource::type = ObjectType::RESOURCE;

std::string Resource::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, value={3}",
      x,
      y,
      "Resource",
      value);
}

ObjectType Resource::getObjectType() const { return type; }

bool Resource::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  // set the resources on the source object
  if (action->getActionType() == ActionType::GATHER) {
    if (sourceObject->getObjectType() == ObjectType::HARVESTER) {
      if (value > 0) {
        value -= 1;
      }
    }
  }
}

Resource::Resource(int value) {
  this->value = value;
}

Resource::~Resource() {}
}  // namespace griddy