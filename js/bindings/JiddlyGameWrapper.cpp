#include "JiddlyGameWrapper.hpp"

#include <emscripten/val.h>

#include <memory>

namespace e = emscripten;

JiddlyGameWrapper::JiddlyGameWrapper(std::string globalObserverName, std::shared_ptr<griddly::GDYFactory> gdyFactory) : gdyFactory_(gdyFactory) {
  std::shared_ptr<griddly::Grid> grid = std::make_shared<griddly::Grid>(griddly::Grid());
  gameProcess_ = std::make_shared<griddly::TurnBasedGameProcess>(griddly::TurnBasedGameProcess(globalObserverName, gdyFactory, grid));
  spdlog::debug("Created game process wrapper");
}

std::shared_ptr<griddly::TurnBasedGameProcess> JiddlyGameWrapper::unwrapped() {
  return gameProcess_;
}

// std::shared_ptr<JiddlyPlayerWrapper> JiddlyGameWrapper::registerPlayer(std::string playerName, std::string observerName) {
//   // auto observerName = Observer::getDefaultObserverName(observerType);
//   auto nextPlayerId = ++playerCount_;
//   auto observer = gdyFactory_->createObserver(gameProcess_->getGrid(), observerName, gdyFactory_->getPlayerCount(), nextPlayerId);

//   auto player = std::make_shared<JiddlyPlayerWrapper>(JiddlyPlayerWrapper(nextPlayerId, playerName, observer, gdyFactory_, gameProcess_));
//   players_.push_back(player);
//   gameProcess_->addPlayer(player->unwrapped());
//   return player;
// }

const uint32_t JiddlyGameWrapper::getActionTypeId(std::string actionName) const {
  auto actionNames = gdyFactory_->getExternalActionNames();
  for (int i = 0; i < actionNames.size(); i++) {
    if (actionNames[i] == actionName) {
      return i;
    }
  }
  throw std::runtime_error("unregistered action");
}

void JiddlyGameWrapper::init() {
  gameProcess_->init(false);
}

void JiddlyGameWrapper::loadLevel(uint32_t levelId) {
  gameProcess_->setLevel(levelId);
}

void JiddlyGameWrapper::loadLevelString(std::string levelString) {
  gameProcess_->setLevel(levelString);
}

void JiddlyGameWrapper::reset() {
  gameProcess_->reset();
}

e::val JiddlyGameWrapper::getGlobalObservationDescription() const {
  return wrapObservationDescription(gameProcess_->getObserver());
}

e::val JiddlyGameWrapper::observe() {
  return wrapObservation(gameProcess_->getObserver());
}

e::val JiddlyGameWrapper::stepParallel(e::val stepArray) {
}

uint32_t JiddlyGameWrapper::getWidth() const {
}

uint32_t JiddlyGameWrapper::getHeight() const {
}

e::val JiddlyGameWrapper::getState() const {
  auto test = e::val::object();
  test.set("x", 5);

  return test;
}

std::vector<std::string> JiddlyGameWrapper::getGlobalVariableNames() const {

}

e::val JiddlyGameWrapper::getObjectVariableMap() const {
  auto test2 = e::val::object();
  auto test = e::val::object();
  test2.set("x", 5);

  test.set("X", test2);

  return test;
}

e::val JiddlyGameWrapper::getGlobalVariables(std::vector<std::string> variables) {

}

std::vector<std::string> JiddlyGameWrapper::getObjectNames() [

]

std::vector<std::string> JiddlyGameWrapper::getObjectVariableNames() {

}

void JiddlyGameWrapper::seedRandomGenerator(uint32_t seed) {

}