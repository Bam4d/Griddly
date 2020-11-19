#pragma once

#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddly {
class Py_GameProcessWrapper {
 public:
  Py_GameProcessWrapper(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory, std::string imagePath, std::string shaderPath)
      : gdyFactory_(gdyFactory),
        imagePath_(imagePath),
        shaderPath_(shaderPath),
        gameProcess_(std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(grid, observer, gdyFactory))) {
    spdlog::debug("Created game process wrapper");
  }

  std::shared_ptr<TurnBasedGameProcess> unwrapped() {
    return gameProcess_;
  }

  std::shared_ptr<Py_StepPlayerWrapper> registerPlayer(std::string playerName, ObserverType observerType) {
    auto observer = createObserver(observerType, gameProcess_->getGrid(), gdyFactory_, imagePath_, shaderPath_);

    auto nextPlayerId = ++numPlayers_;
    auto player = std::shared_ptr<Py_StepPlayerWrapper>(new Py_StepPlayerWrapper(nextPlayerId, playerName, observer, gdyFactory_, gameProcess_));
    gameProcess_->addPlayer(player->unwrapped());
    return player;
  }

  uint32_t getNumPlayers() const {
    return gameProcess_->getNumPlayers();
  }

  py::dict getAvailableActionTypes(int playerId) const {
    auto availableActions = gameProcess_->getAvailableActionTypes(playerId);

    py::dict py_availableActionTypes;
    for (auto availableActionTypesPair : availableActions) {
      auto location = availableActionTypesPair.first;
      auto actionTypes = availableActionTypesPair.second;

      auto relative = actionInputDefinition.relative;
      auto internal = actionInputDefinition.internal;

      py::tuple locationKeyTuple = py::make_tuple(location.x, location.y);
      py_availableActionTypes[locationKeyTuple] = actionTypes;
    }

    return py_availableActionTypes;
  }

  void init() {
    gameProcess_->init();
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> reset() {
    
    auto observation = gameProcess_->reset();
    if (observation != nullptr) {
      auto observer = gameProcess_->getObserver();
      return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), std::move(observation)));
    }

    return nullptr;
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {
    auto observer = gameProcess_->getObserver();

    if (observer == nullptr) {
      throw std::invalid_argument("No global observer configured");
    }

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), gameProcess_->observe(0)));
  }

  std::array<uint32_t, 2> getTileSize() const {
    auto tileSize = gameProcess_->getObserver()->getTileSize();
    return {(uint32_t)tileSize[0], (uint32_t)tileSize[1]};
  }

  // force release of resources for vulkan etc
  void release() {
    gameProcess_->release();
  }

 private:
  const std::shared_ptr<TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::string imagePath_;
  const std::string shaderPath_;
  uint32_t numPlayers_ = 0;
};
}  // namespace griddly