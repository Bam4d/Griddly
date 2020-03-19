#pragma once

#include <memory>
#include <vector>
#include "../Actions/Action.hpp"

namespace griddy {

class GameProcess;

class Player {
 public:
  Player(int id, std::string playerName);

  virtual std::vector<int> performActions(std::vector<std::shared_ptr<Action>> actions);

  std::unique_ptr<uint8_t[]> observe();

  std::string getName() const;
  int getId() const;

  void setGameProcess(std::shared_ptr<GameProcess> gameProcess);

  virtual ~Player() = 0;

 private:
  const int id_;
  const std::string name_;
  std::shared_ptr<GameProcess> gameProcess_;
};
}  // namespace griddy