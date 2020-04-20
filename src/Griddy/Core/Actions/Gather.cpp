#include "Gather.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

Gather::Gather(Direction direction, GridLocation sourceLocation) : direction_(direction),
                                                                   Action(sourceLocation, std::string("Gather"), ActionType::GATHER) {}

Gather::~Gather() {}

std::string Gather::getDescription() const {
  auto destination = getDestinationLocation();
  return fmt::format(
      "{0} [{1}, {2}]->[{3}, {4}]",
      actionTypeName_,
      sourceLocation_.x,
      sourceLocation_.y,
      destination.x,
      destination.y);
}

GridLocation Gather::getDestinationLocation() const {
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

}  // namespace griddy