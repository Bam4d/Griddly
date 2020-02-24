#pragma once

#include "Action.hpp"
namespace griddy {

class Move : public Action {
 public:
  ~Move();
  Move(Direction direction, GridLocation targetLocation);
  std::string getDescription() const override;
  GridLocation getDestinationLocation() const override;

 private:
  const Direction direction_;
};
}  // namespace griddy