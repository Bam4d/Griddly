#include "Player.hpp"

#include <spdlog/spdlog.h>

#include "../GameProcess.hpp"

namespace griddy {

Player::Player(int id, std::string name, std::shared_ptr<Observer> observer)
    : id_(id), name_(name), observer_(observer) {
}

Player::~Player() {}

std::string Player::getName() const {
  return name_;
}

int Player::getId() const {
  return id_;
}

void Player::init(int gridWidth, int gridHeight, std::shared_ptr<GameProcess> gameProcess) {
  spdlog::debug("Initializing player: {0}, name: {1}", id_, name_);
  if (observer_ != nullptr) {
    observer_->init(gridWidth, gridHeight);
  }
  this->gameProcess_ = gameProcess;
}

void Player::reset() {
  if (observer_ != nullptr) {
    observer_->reset();
  }
}

std::shared_ptr<GameProcess> Player::getGameProcess() const {
  return gameProcess_;
}

std::shared_ptr<Observer> Player::getObserver() const {
  return observer_;
}

std::vector<int> Player::performActions(std::vector<std::shared_ptr<Action>> actions) {
  return gameProcess_->performActions(id_, actions);
}

std::unique_ptr<uint8_t[]> Player::observe() {
  if (observer_ == nullptr) {
    return nullptr;
  }
  return observer_->update(id_);
}

}  // namespace griddy