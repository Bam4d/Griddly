#include "Minerals.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"
#include "../Units/Harvester.hpp"

namespace griddy {

const ObjectType Minerals::type = ObjectType::MINERALS;

std::string Minerals::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, value={3}",
      x_,
      y_,
      "Resource",
      value_);
}

ObjectType Minerals::getObjectType() const { return type; }

bool Minerals::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  auto actionType = action->getActionType();
  switch (actionType) {
    case GATHER:
      if (sourceObject->getObjectType() == ObjectType::HARVESTER) {
        if (value_ > 0) {
          value_ -= 1;
          return true;
        }
      }
      return false;
    case MOVE: // Nothing can move to where a resource exists
      return false;
    default:
      return false;
  }

}

uint Minerals::getValue() const {
  return value_;
}

uint Minerals::getMaxValue() const {
  return maxValue_;
}

Minerals::Minerals(uint value): maxValue_(value) {
  value_ = value;
}

Minerals::~Minerals() {}
}  // namespace griddy