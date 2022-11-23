#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../Observers/Observer.hpp"

namespace griddly {

class Action;
class GameProcess;
class Object;
struct ActionResult;

// TODO: this is deprecated
struct DefaultObserverConfig {
  uint32_t overrideGridWidth = 0;
  uint32_t overrideGridHeight = 0;
  int32_t gridXOffset = 0;
  int32_t gridYOffset = 0;
  bool trackAvatar = false;
  bool rotateWithAvatar = false;
  bool rotateAvatarImage = true;
  uint32_t playerCount = 0;
  bool highlightPlayers = true;
};

class Player {
 public:
  Player(uint32_t id, std::string playerName, std::string observerName, std::shared_ptr<GameProcess> gameProcess);

  virtual ActionResult performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks = true);

  virtual void init(std::shared_ptr<Observer> observer);
  virtual void reset(std::shared_ptr<Object> avatarObject = nullptr);

  virtual std::string getName() const;
  virtual uint32_t getId() const;
  virtual std::shared_ptr<int32_t> getScore() const;

  virtual std::shared_ptr<Object> getAvatar();
  virtual std::string getObserverName() const;

  virtual std::shared_ptr<GameProcess> getGameProcess() const;
  virtual std::shared_ptr<Observer> getObserver() const;

  virtual ~Player();

 private:
  const uint32_t id_;
  const std::string name_;
  const std::string observerName_;

  // Is set in direct control situations where the player controls a single avatar
  std::shared_ptr<Object> avatarObject_;
  std::shared_ptr<Observer> observer_;

  // Using a weak ptr here because game process points to the player objects and vice versa
  std::weak_ptr<GameProcess> gameProcess_;
  std::shared_ptr<int32_t> score_;
};
}  // namespace griddly
