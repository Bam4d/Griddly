#pragma once

#include <string>
#include "../Objects/Object.hpp"
#include "../Objects/GridLocation.hpp"

namespace griddy {

class Action {
 public:
  Action(GridLocation targetLocation, std::string actionTypeName);

  GridLocation getTargetLocation() const;
  std::string getActionTypeName() const;

  virtual std::string getDescription() const = 0;
  virtual ~Action() = 0;

 protected:
  const GridLocation targetLocation_;
  const std::string actionTypeName_;
};
}  // namespace griddy