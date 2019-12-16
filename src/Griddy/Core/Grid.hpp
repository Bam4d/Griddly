#pragma once

#include <vector>
#include <memory>
#include "Actions/Action.hpp"

namespace griddy {

class Grid {
 public:
  Grid(int width, int height);
  ~Grid();

  void cloneState();
  void update(std::vector<std::shared_ptr<Action>> actions);

  int getCurrentScore(int playerId);
  int getResources(int playerId);

 private:
  const int height_;
  const int width_;
};

}  // namespace griddy