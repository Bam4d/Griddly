#pragma once

#include <memory>
#include <vector>

namespace griddy {

class Action;
class GameProcess;
class Observer;

class Player {
 public:
  Player(int id, std::string playerName, std::shared_ptr<Observer> observer);

  virtual std::vector<int> performActions(std::vector<std::shared_ptr<Action>> actions);

  void init(int width, int height, std::shared_ptr<GameProcess> gameProcess);
  std::unique_ptr<uint8_t[]> observe();

  std::string getName() const;
  int getId() const;

  std::shared_ptr<GameProcess> getGameProcess() const;

  virtual ~Player();

 private:
  const int id_;
  const std::string name_;
  const std::shared_ptr<Observer> observer_;
  std::shared_ptr<GameProcess> gameProcess_;
  
};
}  // namespace griddy