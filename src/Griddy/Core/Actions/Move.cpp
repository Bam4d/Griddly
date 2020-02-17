#include "Move.hpp"
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

MoveAction::MoveAction(Direction direction, GridLocation targetLocation)
    : direction_(direction),
      Action(targetLocation, std::string("Move")) {}

MoveAction::~MoveAction() {}

std::string MoveAction::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}({3})",
      actionTypeName_,
      targetLocation_.x,
      targetLocation_.y,
      direction_);
}

};  // namespace griddy