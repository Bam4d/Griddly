#include "Object.hpp"

namespace griddy {

GridLocation Object::getLocation() {
  GridLocation location = {x, y};
  return location;
};

} // namespace griddy::grid::objects