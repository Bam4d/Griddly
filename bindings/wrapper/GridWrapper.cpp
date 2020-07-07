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

  std::string getAvatarObject() const {
    return gdyFactory_->getAvatarObject();
  }

  py::dict getActionInputMappings() const {
    auto actionMappings = gdyFactory_->getActionInputsDefinitions();
    py::dict actionInputMappings;
    for (auto actionMapping : actionMappings) {
      auto actionName = actionMapping.first;
      auto mapping = actionMapping.second;

      auto relative = mapping.relative;
      auto internal = mapping.internal;

      py::dict actionInputMapping;

      actionInputMapping["Relative"] = relative;
      actionInputMapping["Internal"] = internal;

      py::dict actionInputDescriptionMap;
      for (auto inputMapping : mapping.inputMap) {

        py::dict actionInputDescription;
        auto inputId = inputMapping.first;
        auto actionInputMapping = inputMapping.second;

        auto vectorToDest = fmt::format("[{0}, {1}]", actionInputMapping.vectorToDest.x, actionInputMapping.vectorToDest.y);
        auto orientationVector = fmt::format("[{0}, {1}]", actionInputMapping.orientationVector.x, actionInputMapping.orientationVector.y);

        actionInputDescription["Description"] = actionInputMapping.description;
        actionInputDescription["VectorToDest"] = vectorToDest;
        actionInputDescription["OrientationVector"] = orientationVector;
        actionInputDescriptionMap[std::to_string(inputId).c_str()] = actionInputDescription;
      }

      actionInputMapping["InputMapping"] = actionInputDescriptionMap;

      actionInputMappings[actionName.c_str()] = actionInputMapping;
    }

    return actionInputMappings;
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