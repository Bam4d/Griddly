#include "MoveAction.hpp"
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

MoveAction::MoveAction(Direction direction, GridLocation targetLocation)
    : direction_(direction),
      Action(targetLocation, std::string("MoveAction")) {}

MoveAction::~MoveAction() {}

std::string MoveAction::log() const {
  return fmt::format("[{0}, {1}] {2}({3})", targetLocation_.x,
                     targetLocation_.y, actionTypeName_, direction_);
}

};  // namespace griddy