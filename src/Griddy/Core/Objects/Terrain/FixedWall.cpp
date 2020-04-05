#include "FixedWall.hpp"
#include <spdlog/fmt/fmt.h>
#include "../../Actions/Action.hpp"
#include "../Units/Pusher.hpp"
#include "../../Grid.hpp"

namespace griddy {

const ObjectType FixedWall::type = ObjectType::FIXED_WALL;

std::string FixedWall::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}, value={3}",
      x,
      y,
      "FixedWall",
      value);
}

bool FixedWall::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  return false;
}

ObjectType FixedWall::getObjectType() const { return type; }

FixedWall::FixedWall() {
}

FixedWall::~FixedWall() {}
}  // namespace griddy