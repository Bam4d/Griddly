#pragma once
#include <emscripten/val.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectRules.hpp"
#include "../../src/Griddly/Core/GameProcess.hpp"
#include "../../src/Griddly/Core/Observers/TensorObservationInterface.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

namespace e = emscripten;

class GriddlyJSPlayerWrapper {
 public:
  GriddlyJSPlayerWrapper(int playerId, std::string playerName, std::string observerName, std::shared_ptr<griddly::GDYFactory> gdyFactory, std::shared_ptr<griddly::GameProcess> gameProcess);

  std::shared_ptr<griddly::Player> unwrapped();
  e::val getObservationDescription() const;
  e::val observe();
  e::val observeImage();

  e::val stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks);

  // Build an engine action for this player (nullptr for an unknown action id)
  std::shared_ptr<griddly::Action> buildAction(std::string actionName, std::vector<int32_t> actionArray);

  static e::val buildInfo(griddly::ActionResult actionResult);

 private:
  const std::shared_ptr<griddly::Player> player_;
  const std::shared_ptr<griddly::GDYFactory> gdyFactory_;
  const std::shared_ptr<griddly::GameProcess> gameProcess_;

};