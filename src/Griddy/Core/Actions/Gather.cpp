#include "Gather.hpp"
#include <spdlog/fmt/fmt.h>
#include <string>

namespace griddy {

Gather::Gather(GridLocation targetLocation) : Action(targetLocation, std::string("Gather")) {}

Gather::~Gather() {}

std::string Gather::getDescription() const {
  return fmt::format(
      "[{0}, {1}] {2}",
      actionTypeName_,
      targetLocation_.x,
      targetLocation_.y);
}
}  // namespace griddy