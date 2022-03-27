#include "JiddlyPlayerWrapper.hpp"

JiddlyPlayerWrapper::JiddlyPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<griddly::Observer> observer, std::shared_ptr<griddly::GDYFactory> gdyFactory, std::shared_ptr<griddly::GameProcess> gameProcess)
    : player_(std::make_shared<griddly::Player>(griddly::Player(playerId, playerName, observer, gameProcess))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
}

std::shared_ptr<griddly::Player> JiddlyPlayerWrapper::unwrapped() {
  return player_;
}
