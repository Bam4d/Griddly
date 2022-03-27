#pragma once
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "JiddlyPlayerWrapper.hpp"

namespace e = emscripten;

class JiddlyGameWrapper {
 public:
  JiddlyGameWrapper(std::string globalObserverName, std::shared_ptr<griddly::GDYFactory> gdyFactory);

  std::shared_ptr<griddly::TurnBasedGameProcess> unwrapped();

  std::shared_ptr<JiddlyPlayerWrapper> registerPlayer(std::string playerName, std::string observerName);

  const uint32_t getActionTypeId(std::string actionName) const;

  void init();

  void loadLevel(uint32_t levelId);

  void loadLevelString(std::string levelString);

  void reset();

  e::val getGlobalObservationDescription() const;

  e::val observe();

  e::val stepParallel(e::val stepArray);

  uint32_t getWidth() const;

  uint32_t getHeight() const;

  e::val getState() const;

  std::vector<std::string> getGlobalVariableNames() const;

  e::val getObjectVariableMap() const;

  e::val getGlobalVariables(std::vector<std::string> variables);

  std::vector<std::string> getObjectNames();

  std::vector<std::string> getObjectVariableNames();

  void seedRandomGenerator(uint32_t seed);

 private:
  std::shared_ptr<griddly::TurnBasedGameProcess> gameProcess_;
  const std::shared_ptr<griddly::GDYFactory> gdyFactory_;
  uint32_t playerCount_ = 0;
  std::vector<std::shared_ptr<JiddlyPlayerWrapper>> players_;
};