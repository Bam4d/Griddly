#include <spdlog/spdlog.h>

#include <utility>

#include "../GameProcess.hpp"
#include "../Observers/ObservationInterface.hpp"
#include "Player.hpp"

namespace griddly {

Player::Player(uint32_t id, std::string name, std::string observerName, std::shared_ptr<GameProcess> gameProcess)
    : id_(id), name_(std::move(name)), observerName_(std::move(observerName)) {
  score_ = std::make_shared<int32_t>(0);

  gameProcess_ = gameProcess;
  *score_ = 0;
}

Player::~Player() {
  spdlog::debug("Player Destroyed");
}

std::string Player::getName() const {
  return name_;
}

uint32_t Player::getId() const {
  return id_;
}

std::shared_ptr<int32_t> Player::getScore() const {
  return score_;
}

std::string Player::getObserverName() const {
  return observerName_;
}

void Player::init(std::shared_ptr<Observer> observer) {
  observer_ = observer;
}

void Player::reset(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;

  if (observer_ != nullptr) {
    observer_->reset(avatarObject);
  }

  *score_ = 0;
}

std::shared_ptr<Object> Player::getAvatar() {
  return avatarObject_;
}

std::shared_ptr<GameProcess> Player::getGameProcess() const {
  return gameProcess_.lock();
}

std::shared_ptr<Observer> Player::getObserver() const {
  return observer_;
}

ActionResult Player::performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
  return gameProcess_.lock()->performActions(id_, actions, updateTicks);
}

}  // namespace griddly
