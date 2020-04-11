#include "Base.hpp"

#include <spdlog/fmt/fmt.h>

#include "../../Actions/Action.hpp"
#include "../Units/Harvester.hpp"

namespace griddy {

const ObjectType Base::type = ObjectType::BASE;

std::string Base::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, value={3}",
      x,
      y,
      "Base",
      minerals_);
}

ObjectType Base::getObjectType() const { return type; }

bool Base::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  auto actionType = action->getActionType();
  switch (actionType) {
    case GATHER:
      if (sourceObject->getObjectType() == ObjectType::HARVESTER) {
        auto harvester = std::static_pointer_cast<Harvester>(sourceObject);
        if (harvester->getMinerals() > 0) {
          minerals_ += 1;
          return true;
        }
      }
      return false;
    case MOVE:  // Nothing can move to where a resource exists
      return false;
    default:
      return false;
  }
}

int Base::getMinerals() const {
  return minerals_;
}

Base::Base(int playerId) : Unit(playerId, 10) {
}

Base::~Base() {}
}  // namespace griddy