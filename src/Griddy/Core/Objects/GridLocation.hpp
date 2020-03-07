#pragma once
#include <cstdio>
#include <functional>
#include "../Util/util.hpp"

namespace griddy {

class GridLocation {
 public:
  GridLocation(int x, int y) : x(x), y(y) {}

  bool operator==(const GridLocation& other) const {
    return x == other.x && y == other.y;
  }

  struct Hash {
  std::size_t operator()(GridLocation const& location) const noexcept {
    std::size_t seed = 0;
    hash_combine(seed, location.x);
    hash_combine(seed, location.y);
    return seed;
  }
};

  const int x;
  const int y;
};



}  // namespace griddy
