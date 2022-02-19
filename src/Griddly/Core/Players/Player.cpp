#include <spdlog/spdlog.h>

#include "../GameProcess.hpp"
#include "Player.hpp"
#include "../Observers/ObservationInterface.hpp"

namespace griddly {

Player::Player(uint32_t id, std::string name, std::shared_ptr<Observer> observer, std::shared_ptr<GameProcess> gameProcess)
    : id_(id), name_(name), observer_(observer) {
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

void Player::reset() {
  if (observer_ != nullptr) {
    observer_->reset();
  }

  *score_ = 0;
}

void Player::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;
  if (observer_->trackAvatar()) {
    observer_->setAvatar(avatarObject);
  }
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