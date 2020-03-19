#include "Player.hpp"
#include "../GameProcess.hpp"

namespace griddy {

Player::Player(int id, std::string name)
    : id_(id), name_(name) {
}

Player::~Player() {}

std::string Player::getName() const {
  return name_;
}

int Player::getId() const {
  return id_;
}

void Player::setGameProcess(std::shared_ptr<GameProcess> gameProcess) {
  this->gameProcess_ = gameProcess;
}

std::vector<int> Player::performActions(std::vector<std::shared_ptr<Action>> actions) {
  return gameProcess_->performActions(id_, actions);
}

std::unique_ptr<uint8_t[]> Player::observe() {
  return gameProcess_->observe(id_);
}

}  // namespace griddy