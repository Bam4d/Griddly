#pragma once

#include <string>
#include "../../Objects/Object.hpp"

namespace griddy {

class Action {
 public:
  Action(GridLocation targetLocation, std::string actionTypeName);

  GridLocation getTargetLocation() const;
  std::string getActionTypeName() const;

  virtual std::string log() const = 0;
  virtual ~Action() = 0;

 protected:
  const GridLocation targetLocation_;
  const std::string actionTypeName_;
};
}  // namespace griddy