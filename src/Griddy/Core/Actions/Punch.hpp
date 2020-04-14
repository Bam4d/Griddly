#pragma once

#include "Action.hpp"

namespace griddy {
class Punch : public Action {
 public:
  Punch(Direction direction, GridLocation sourceLocation);
  ~Punch();
  std::string getDescription() const override;
  GridLocation getDestinationLocation() const override;

 private:
  const Direction direction_;
};

}  // namespace griddy