#pragma once

#include <spdlog/spdlog.h>

#include "../../src/Griddle/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddle {
class Py_GameProcessWrapper {
 public:
  Py_GameProcessWrapper(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory)
      : gdyFactory_(gdyFactory), gameProcess_(std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(grid, observer, gdyFactory))) {
    spdlog::debug("Created game process wrapper");
  }

  std::shared_ptr<TurnBasedGameProcess> unwrapped() {
    return gameProcess_;
  }

  std::shared_ptr<Py_StepPlayerWrapper> addPlayer(std::string playerName, ObserverType observerType) {
    auto observer = createObserver(observerType, gameProcess_->getGrid(), gdyFactory_);

    auto nextPlayerId = ++numPlayers_;
    auto player = std::shared_ptr<Py_StepPlayerWrapper>(new Py_StepPlayerWrapper(nextPlayerId, playerName, observer));
    gameProcess_->addPlayer(player->unwrapped());
    return player;
  }

  void init() {
    gameProcess_->init();
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> reset() {
    auto observer = gameProcess_->getObserver();

    if (observer != nullptr) {
      auto observation = gameProcess_->reset();
      return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), std::move(observation)));
    }

    return nullptr;
    
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {
    auto observer = gameProcess_->getObserver();

    if (observer == nullptr) {
      throw std::invalid_argument("No global observer configured");
    }

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), gameProcess_->observe(-1)));
  }

 private:
  const std::shared_ptr<TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  uint32_t numPlayers_ = 0;
};
}  // namespace griddle