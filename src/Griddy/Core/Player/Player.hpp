#pragma once

#include <memory>
#include <vector>
#include "../Actions/Action.hpp"
#include "../GameProcess.hpp"

namespace griddy {

class Player : std::enable_shared_from_this<Player> {
 public:
  Player(int id, std::string playerName, std::shared_ptr<GameProcess> gameProcess);

  virtual void act(std::vector<std::shared_ptr<Action>> actions) = 0;

  std::unique_ptr<uint8_t[]> observe();

  std::string getName() const;
  int getId() const;

  virtual ~Player() = 0;

 private:
  const int id_;
  const std::string name_;
  const std::shared_ptr<GameProcess> gameProcess_;
};
}  // namespace griddy