#pragma once
#include <emscripten/val.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Observers/TensorObservationInterface.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

class JiddlyPlayerWrapper {
 public:
  JiddlyPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<griddly::Observer> observer, std::shared_ptr<griddly::GDYFactory> gdyFactory, std::shared_ptr<griddly::GameProcess> gameProcess);

  std::shared_ptr<griddly::Player> unwrapped();

 private:
  const std::shared_ptr<griddly::Player> player_;
  const std::shared_ptr<griddly::GDYFactory> gdyFactory_;
  const std::shared_ptr<griddly::GameProcess> gameProcess_;
};