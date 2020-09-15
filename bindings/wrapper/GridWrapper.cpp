#pragma once

#include <pybind11/pybind11.h>
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
  Py_GridWrapper(std::shared_ptr<GDYFactory> gdyFactory, std::string imagePath, std::string shaderPath)
      : grid_(std::shared_ptr<Grid>(new Grid())),
        gdyFactory_(gdyFactory),
        imagePath_(imagePath),
        shaderPath_(shaderPath) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  std::array<uint32_t, 2> getTileSize() const {
    return std::array<uint32_t, 2>{(uint32_t)gdyFactory_->getTileSize()[0], (uint32_t)gdyFactory_->getTileSize()[1]};
  }

  void setTileSize(std::array<uint32_t, 2> py_tileSize) {
    if (isBuilt_) {
      throw std::invalid_argument("Cannot set tile size after game has been created.");
    }
    gdyFactory_->overrideTileSize(glm::ivec2(py_tileSize[0], py_tileSize[1]));
  }

  void enableHistory(bool enable) {
    grid_->enableHistory(enable);
  }

  void setMaxSteps(uint32_t maxSteps) {
    gdyFactory_->setMaxSteps(maxSteps);
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

  py::dict getActionInputMappings() const {
    auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
    py::dict py_actionInputsDefinitions;
    for (auto actionInputDefinitionPair : actionInputsDefinitions) {
      auto actionName = actionInputDefinitionPair.first;
      auto actionInputDefinition = actionInputDefinitionPair.second;

      auto relative = actionInputDefinition.relative;
      auto internal = actionInputDefinition.internal;

      py::dict py_actionInputsDefinition;

      py_actionInputsDefinition["Relative"] = relative;
      py_actionInputsDefinition["Internal"] = internal;

      py::dict py_actionInputMappings;
      for (auto inputMapping : actionInputDefinition.inputMappings) {

        py::dict py_actionInputMapping;
        auto inputId = inputMapping.first;
        auto actionInputMapping = inputMapping.second;

        auto vectorToDest = fmt::format("[{0}, {1}]", actionInputMapping.vectorToDest.x, actionInputMapping.vectorToDest.y);
        auto orientationVector = fmt::format("[{0}, {1}]", actionInputMapping.orientationVector.x, actionInputMapping.orientationVector.y);

        py_actionInputMapping["Description"] = actionInputMapping.description;
        py_actionInputMapping["VectorToDest"] = vectorToDest;
        py_actionInputMapping["OrientationVector"] = orientationVector;
        py_actionInputMappings[std::to_string(inputId).c_str()] = py_actionInputMapping;
      }

      py_actionInputsDefinition["InputMappings"] = py_actionInputMappings;

      py_actionInputsDefinitions[actionName.c_str()] = py_actionInputsDefinition;
    }

    return py_actionInputsDefinitions;
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