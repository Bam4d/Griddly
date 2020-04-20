#include "Move.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

Move::Move(Direction direction, GridLocation sourceLocation) : direction_(direction),
                                                               Action(sourceLocation, std::string("Move"), ActionType::MOVE) {}

Move::~Move() {}

Direction Move::getDirection() const {
  return direction_;
}

std::string Move::getDescription() const {
  auto destination = getDestinationLocation();
  return fmt::format(
      "{0} [{1}, {2}]->[{3}, {4}]",
      actionTypeName_,
      sourceLocation_.x,
      sourceLocation_.y,
      destination.x,
      destination.y);
}

GridLocation Move::getDestinationLocation() const {
  switch (direction_) {
    case UP:
      return {
          sourceLocation_.x,
          sourceLocation_.y + 1};
    case RIGHT:
      return {
          sourceLocation_.x + 1,
          sourceLocation_.y};
    case DOWN:
      return {
          sourceLocation_.x,
          sourceLocation_.y - 1};
    case LEFT:
      return {
          sourceLocation_.x - 1,
          sourceLocation_.y};
  }
}

};  // namespace griddy