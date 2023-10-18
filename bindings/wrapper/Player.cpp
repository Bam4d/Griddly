#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/GameProcess.hpp"
#include "Griddly/Core/Observers/TensorObservationInterface.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "WrapperCommon.cpp"

namespace py = pybind11;

namespace griddly {
class Py_Player {
 public:
  Py_Player(int playerId, std::string playerName, std::string observerName, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess)
      : player_(std::make_shared<Player>(Player(playerId, playerName, observerName, gameProcess))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
  }

  ~Py_Player() {
    spdlog::trace("StepPlayerWrapper Destroyed");
  }

  std::shared_ptr<Player> unwrapped() {
    return player_;
  }

  py::object getObservationDescription() const {
    return wrapObservationDescription(player_->getObserver());
  }

  py::object observe() {
    return wrapObservation(player_->getObserver());
  }

  ActionResult performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
    return player_->performActions(actions, updateTicks);
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;
};

}  // namespace griddly