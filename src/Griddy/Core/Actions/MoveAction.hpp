#pragma once

#include "Action.hpp"
namespace griddy {

enum Direction { UP, DOWN, LEFT, RIGHT };

class MoveAction : public Action {
 public:
  ~MoveAction();
  MoveAction(Direction direction, GridLocation targetLocation);
  std::string log() const override;

 private:
  const Direction direction_;
};
}  // namespace griddy