#pragma once

#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "GameProcessWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddly {

class Py_GridWrapper {
 public:
  Py_GridWrapper(std::shared_ptr<Grid> grid, std::shared_ptr<GDYFactory> gdyFactory, std::string imagePath, std::string shaderPath)
      : grid_(grid),
        gdyFactory_(gdyFactory),
        imagePath_(imagePath),
        shaderPath_(shaderPath) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  uint32_t getTileSize() const {
    return gdyFactory_->getTileSize();
  }

  uint32_t getWidth() const {
    return grid_->getWidth();
  }

  uint32_t getHeight() const {
    return grid_->getHeight();
  }

  uint32_t getPlayerCount() const {
    return gdyFactory_->getPlayerCount();
  }

  std::string getActionNameFromId(uint32_t actionDefinitionIdx) const {
    return gdyFactory_->getActionName(actionDefinitionIdx);
  }

  uint32_t getActionDefinitionCount() const {
    return gdyFactory_->getActionDefinitionCount();
  }

  std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>> getActionInputMappings() const {
    return gdyFactory_->getActionInputMappings();
  }

  void addObject(int playerId, int32_t startX, int32_t startY, std::string objectName) {
    auto objectGenerator = gdyFactory_->getObjectGenerator();

    auto object = objectGenerator->newInstance(objectName, grid_->getGlobalVariables());

    grid_->initObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Py_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_, gdyFactory_, imagePath_, shaderPath_);

    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(grid_, globalObserver, gdyFactory_, imagePath_, shaderPath_));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::string imagePath_;
  const std::string shaderPath_;

  bool isBuilt_ = false;
};

}  // namespace griddly