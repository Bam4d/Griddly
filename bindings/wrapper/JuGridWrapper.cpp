#pragma once

#include <spdlog/spdlog.h>
#include <map>
#include <memory>
#include <any>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "JuGameProcessWrapper.cpp"
#include "JuStepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddly {

class Ju_GridWrapper {
 public:
  Ju_GridWrapper(std::shared_ptr<GDYFactory> gdyFactory, std::string imagePath, std::string shaderPath)
      : grid_(std::shared_ptr<Grid>(new Grid())),
        gdyFactory_(gdyFactory),
        imagePath_(imagePath),
        shaderPath_(shaderPath) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  void setTileSize(uint32_t tileSize) {
    if (isBuilt_) {
      throw std::invalid_argument("Cannot set tile size after game has been created.");
    }
    gdyFactory_->overrideTileSize(tileSize);
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

  std::string getAvatarObject() const {
    return gdyFactory_->getAvatarObject();
  }

  std::vector<std::string> getAllAvailableAction() {
      return gdyFactory_->getAllAvailableAction();
  };

  std::vector<std::string> getPlayerAvailableAction() {
      return gdyFactory_->getPlayerAvailableAction();
  };

  std::vector<std::string> getNonPlayerAvailableAction() {
      return gdyFactory_->getNonPlayerAvailableAction();
  };

  std::vector<uint32_t> getInputsIds(std::string actionName) {
      return gdyFactory_->getInputsIds(actionName);
  }

  void createLevel(uint32_t width, uint32_t height) {
    gdyFactory_->createLevel(width, height, grid_);
  }

  void loadLevel(uint32_t level) {
    gdyFactory_->loadLevel(level);
  }

  void loadLevelString(std::string levelString) {
    gdyFactory_->loadLevelString(levelString);
  }

  void addObject(int playerId, int32_t startX, int32_t startY, std::string objectName) {
    auto objectGenerator = gdyFactory_->getObjectGenerator();

    auto object = objectGenerator->newInstance(objectName, grid_->getGlobalVariables());

    grid_->initObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Ju_GameProcessWrapper> createGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Already created a game using this grid.");
    }

    isBuilt_ = true;

    auto globalObserver = createObserver(observerType, grid_, gdyFactory_, imagePath_, shaderPath_);

    return std::shared_ptr<Ju_GameProcessWrapper>(new Ju_GameProcessWrapper(grid_, globalObserver, gdyFactory_, imagePath_, shaderPath_));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::string imagePath_;
  const std::string shaderPath_;

  bool isBuilt_ = false;
};

}  // namespace griddly