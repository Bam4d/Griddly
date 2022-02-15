#include "Player.hpp"

#include <spdlog/spdlog.h>

#include <utility>

#include "../GameProcess.hpp"

namespace griddly {

Player::Player(uint32_t id, std::string name, std::shared_ptr<Observer> observer)
    : id_(id), name_(std::move(name)), observer_(std::move(observer)) {
  score_ = std::make_shared<int32_t>(0);
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

void Player::init(ObserverConfig observerConfig, bool trackAvatar, std::shared_ptr<GameProcess> gameProcess) {
  spdlog::debug("Initializing player: {0}, name: {1}", id_, name_);

  if (observer_ != nullptr) {
    observerTracksAvatar_ = trackAvatar;
    observer_->init(observerConfig);
  }
  this->gameProcess_ = gameProcess;

  *score_ = 0;
}

void Player::reset() {
  if (observer_ != nullptr) {
    observer_->reset();
  }

  *score_ = 0;
}

void Player::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_ = avatarObject;
  if (observerTracksAvatar_) {
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

uint8_t* Player::observe() {
  return observer_->update();
}

}  // namespace griddly