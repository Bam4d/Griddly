#pragma once
#include <emscripten/val.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "GriddlyJSGameWrapper.hpp"

namespace e = emscripten;

class GriddlyJSGDYWrapper {
 public:
  GriddlyJSGDYWrapper(std::shared_ptr<griddly::GDYFactory> gdyFactory);

  void setMaxSteps(uint32_t maxSteps);

  uint32_t getPlayerCount() const;

  std::string getAvatarObject() const;

  e::val getExternalActionNames() const;

  uint32_t getLevelCount() const;

  griddly::ObserverType& getObserverType(std::string observerName);

  e::val getActionInputMappings() const;

  std::shared_ptr<GriddlyJSGameWrapper> createGame(std::string globalObserverName);

 private:
  const std::shared_ptr<griddly::GDYFactory> gdyFactory_;
};
