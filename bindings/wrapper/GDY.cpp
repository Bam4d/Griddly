#pragma once

#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "GameProcess.cpp"
#include "Player.cpp"

namespace griddly {

class Py_GDY {
 public:
  Py_GDY(std::shared_ptr<GDYFactory> gdyFactory)
      : gdyFactory_(gdyFactory) {
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

  std::vector<std::string> getExternalActionNames() const {
    return gdyFactory_->getExternalActionNames();
  }

  uint32_t getLevelCount() const {
    return gdyFactory_->getLevelCount();
  }

  ObserverType& getObserverType(std::string observerName) {
    return gdyFactory_->getNamedObserverType(observerName);
  }

  py::dict getActionInputMappings() const {
    const auto& actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
    py::dict py_actionInputsDefinitions;
    for (auto actionInputDefinitionPair : actionInputsDefinitions) {
      auto actionName = actionInputDefinitionPair.first;
      auto actionInputDefinition = actionInputDefinitionPair.second;
      
      auto internal = actionInputDefinition.internal;
      auto relative = actionInputDefinition.relative;
      auto mapToGrid = actionInputDefinition.mapToGrid;

      py::dict py_actionInputsDefinition;

      py_actionInputsDefinition["Relative"] = relative;
      py_actionInputsDefinition["Internal"] = internal;
      py_actionInputsDefinition["MapToGrid"] = mapToGrid;

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

  std::shared_ptr<Py_GameProcess> createGame(std::string globalObserverName) {
    return std::make_shared<Py_GameProcess>(Py_GameProcess(globalObserverName, gdyFactory_));
  }

 private:
  const std::shared_ptr<GDYFactory> gdyFactory_;
};

}  // namespace griddly