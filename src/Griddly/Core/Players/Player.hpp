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

struct PlayerObserverDefinition {
  uint32_t gridWidth = 0;
  uint32_t gridHeight = 0;
  uint32_t gridXOffset = 0;
  uint32_t gridYOffset = 0;
  bool trackAvatar = false;
  bool rotateWithAvatar = false;
  uint32_t playerCount;
};

class Player {
 public:
  Player(uint32_t id, std::string playerName, std::shared_ptr<Observer> observer);

  virtual ActionResult performActions(std::vector<std::shared_ptr<Action>> actions);

  virtual void init(ObserverConfig observerConfig, bool trackAvatar, std::shared_ptr<GameProcess> gameProcess);
  virtual void reset();

  virtual std::shared_ptr<uint8_t> observe();

  virtual std::string getName() const;
  virtual uint32_t getId() const;
  virtual std::shared_ptr<int32_t> getScore() const;

  virtual std::shared_ptr<Object> getAvatar();
  virtual void setAvatar(std::shared_ptr<Object> avatarObject);

  virtual std::shared_ptr<GameProcess> getGameProcess() const;
  virtual std::shared_ptr<Observer> getObserver() const;

  virtual ~Player();

 private:
  const uint32_t id_;
  const std::string name_;
  bool observerTracksAvatar_ = false;

  // Is set in direct control situations where the player controls a single avatar
  std::shared_ptr<Object> avatarObject_;

  const std::shared_ptr<Observer> observer_;
  std::shared_ptr<GameProcess> gameProcess_;
  std::shared_ptr<int32_t> score_;
};
}  // namespace griddly