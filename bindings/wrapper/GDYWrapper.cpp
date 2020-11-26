#pragma once

#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "GameWrapper.cpp"
#include "StepPlayerWrapper.cpp"
#include "wrapper.hpp"

namespace griddly {

class Py_GDYWrapper {
 public:
  Py_GDYWrapper(std::shared_ptr<GDYFactory> gdyFactory, std::string imagePath, std::string shaderPath)
      : grid_(std::shared_ptr<Grid>(new Grid())),
        gdyFactory_(gdyFactory),
        imagePath_(imagePath),
        shaderPath_(shaderPath) {
    // Do not need to init the grid here as the level generator will take care of that when the game process is created
  }

  void setMaxSteps(uint32_t maxSteps) {
    gdyFactory_->setMaxSteps(maxSteps);
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

        auto vectorToDest = py::cast(std::vector<int32_t>{actionInputMapping.vectorToDest.x, actionInputMapping.vectorToDest.y});
        auto orientationVector = py::cast(std::vector<int32_t>{actionInputMapping.orientationVector.x, actionInputMapping.orientationVector.y});

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

  void addObject(int playerId, int32_t startX, int32_t startY, std::string objectName) {
    auto objectGenerator = gdyFactory_->getObjectGenerator();

    auto object = objectGenerator->newInstance(objectName, grid_->getGlobalVariables());

    grid_->addObject(playerId, {startX, startY}, object);
  }

  std::shared_ptr<Py_GameWrapper> createGame(ObserverType observerType) {

    auto globalObserver = createObserver(observerType, gdyFactory_, imagePath_, shaderPath_);

    return std::shared_ptr<Py_GameWrapper>(new Py_GameWrapper(globalObserver, gdyFactory_, imagePath_, shaderPath_));
  }

 private:
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::string imagePath_;
  const std::string shaderPath_;
};

}  // namespace griddly