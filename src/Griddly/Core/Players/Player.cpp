#include "Player.hpp"

#include <spdlog/spdlog.h>

#include "../GameProcess.hpp"

namespace griddly {

Player::Player(uint32_t id, std::string name, std::shared_ptr<Observer> observer)
    : id_(id), name_(name), observer_(observer) {
  score_ = std::make_shared<int32_t>(0);
}

Player::~Player() {}

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
  if(observerTracksAvatar_) {
    observer_->setAvatar(avatarObject);
  }
}

std::shared_ptr<Object> Player::getAvatar() {
  return avatarObject_;
}

std::shared_ptr<GameProcess> Player::getGameProcess() const {
  return gameProcess_;
}

std::shared_ptr<Observer> Player::getObserver() const {
  return observer_;
}

ActionResult Player::performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
  auto actionResult = gameProcess_->performActions(id_, actions, updateTicks);

  // Update the player's score
  for (auto r : actionResult.rewards) {
    *score_ += r;
  }

  return actionResult;
}

std::shared_ptr<uint8_t> Player::observe() {
  if (observer_ == nullptr) {
    return nullptr;
  }
  return observer_->update();
}

}  // namespace griddly