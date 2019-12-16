#pragma once

namespace griddy {

struct GridLocation {
  const int& x;
  const int& y;
};

class Object {
 public:
  Object(int x, int y) {
    this->x = x;
    this->y = y;
  }

  GridLocation getLocation();

  virtual ~Object()=0;

 private:
  int x;
  int y;
};

}  // namespace griddy::grid::objects