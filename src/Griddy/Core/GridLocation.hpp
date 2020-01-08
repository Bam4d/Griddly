#pragma once
#include <cstdio>
#include <functional>
#include "Util/util.hpp"

namespace griddy {
class GridLocation {
 public:
  GridLocation(int x, int y) : x(x), y(y) {}

  bool operator==(const GridLocation& other) const {
    return x == other.x && y == other.y;
  }

  // custom hash can be a standalone function object:
  struct Hash {
    std::size_t operator()(GridLocation const& location) const noexcept {
      std::size_t h1 = std::hash<int>{}(location.x);
      std::size_t h2 = std::hash<int>{}(location.y);
      return h1 ^ (h2 << 1);  // or use boost::hash_combine (see Discussion)
    }
  };

  const int x;
  const int y;
};
}  // namespace griddy
