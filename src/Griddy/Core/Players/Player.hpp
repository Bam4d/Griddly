#pragma once

#include <string>
#include <memory>
#include <vector>

namespace griddy {

class Action;
class GameProcess;
class Observer;

class Player {
 public:
  Player(uint32_t id, std::string playerName, std::shared_ptr<Observer> observer);

  virtual std::vector<int> performActions(std::vector<std::shared_ptr<Action>> actions);

  void init(uint32_t width, uint32_t height, std::shared_ptr<GameProcess> gameProcess);
  void reset();

  virtual std::unique_ptr<uint8_t[]> observe();

  virtual std::string getName() const;
  virtual uint32_t getId() const;
  virtual std::shared_ptr<int32_t> getScore() const;

  virtual std::shared_ptr<GameProcess> getGameProcess() const;
  virtual std::shared_ptr<Observer> getObserver() const;

  virtual ~Player();

 private:
  const uint32_t id_;
  const std::string name_;
  const std::shared_ptr<Observer> observer_;
  std::shared_ptr<GameProcess> gameProcess_;
  std::shared_ptr<int32_t> score_;
  
};
}  // namespace griddy