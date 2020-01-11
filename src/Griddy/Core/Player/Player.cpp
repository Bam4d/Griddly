#include "Player.hpp"

namespace griddy {

Player::Player(int id, std::string name, std::shared_ptr<GameProcess> gameProcess)
    : gameProcess_(gameProcess), id_(id), name_(name) {
}

Player::~Player() {}

std::string Player::getName() const {
  return name_;
}

int Player::getId() const {
  return id_;
}

std::unique_ptr<uint8_t[]> Player::observe() {
  return gameProcess_->observe(shared_from_this());
}

}  // namespace griddy