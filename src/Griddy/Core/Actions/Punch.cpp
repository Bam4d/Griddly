#include "Punch.hpp"
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

Punch::Punch(Direction direction, GridLocation targetLocation) : direction_(direction),
                                                                   Action(targetLocation, std::string("Punch"), ActionType::PUNCH) {}

Punch::~Punch() {}

std::string Punch::getDescription() const {
  auto destination = getDestinationLocation();
  return fmt::format(
      "{0} [{1}, {2}]->[{3}, {4}]",
      actionTypeName_,
      targetLocation_.x,
      targetLocation_.y,
      destination.x,
      destination.y);
}

GridLocation Punch::getDestinationLocation() const {
  switch (direction_) {
    case UP:
      return {
          targetLocation_.x,
          targetLocation_.y + 1};
    case RIGHT:
      return {
          targetLocation_.x + 1,
          targetLocation_.y};
    case DOWN:
      return {
          targetLocation_.x,
          targetLocation_.y - 1};
    case LEFT:
      return {
          targetLocation_.x - 1,
          targetLocation_.y};
  }
}

}  // namespace griddy