#include "Player.hpp"
#include "../GameProcess.hpp"

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

int act(std::vector<std::shared_ptr<Action>> actions) {

}

std::unique_ptr<uint8_t[]> Player::observe() const {
  return gameProcess_->observe(shared_from_this());
}

}  // namespace griddy