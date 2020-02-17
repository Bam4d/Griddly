#pragma once

#include "Action.hpp"

namespace griddy {
class Gather : public Action {
 public:
  Gather(GridLocation targetLocation);
  ~Gather();
  std::string getDescription() const override;
};

}  // namespace griddy