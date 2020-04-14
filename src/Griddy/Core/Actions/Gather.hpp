#pragma once

#include "Action.hpp"

namespace griddy {
class Gather : public Action {
 public:
  Gather(Direction direction, GridLocation sourceLocation);
  ~Gather();
  std::string getDescription() const override;
  GridLocation getDestinationLocation() const override;

 private:
  const Direction direction_;
};

}  // namespace griddy