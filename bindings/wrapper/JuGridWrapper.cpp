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

  /*std::map<std::string,std::any> getActionInputMappings() const {
     auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
     std::map<std::string, std::any> py_actionInputsDefinitions;
     for (auto actionInputDefinitionPair : actionInputsDefinitions) {
       auto actionName = actionInputDefinitionPair.first;
       auto actionInputDefinition = actionInputDefinitionPair.second;

       auto relative = actionInputDefinition.relative;
       auto internal = actionInputDefinition.internal;

       static std::map<std::string, std::any> py_actionInputsDefinition = {
           {"Relative",relative},
           {"Internal",internal}
       };

       std::map<std::string, std::map<std::string, std::string>> py_actionInputMappings;
       for (auto inputMapping : actionInputDefinition.inputMappings) {

         std::map<std::string, std::string> py_actionInputMapping;
         auto inputId = inputMapping.first;
         auto actionInputMapping = inputMapping.second;

         auto vectorToDest = fmt::format("[{0}, {1}]", actionInputMapping.vectorToDest.x, actionInputMapping.vectorToDest.y);
         auto orientationVector = fmt::format("[{0}, {1}]", actionInputMapping.orientationVector.x, actionInputMapping.orientationVector.y);

         py_actionInputMapping = {
             {"Description",actionInputMapping.description},
             {"VectorToDest", vectorToDest},
             {"OrientationVector", orientationVector}
         };

         py_actionInputMappings[std::to_string(inputId).c_str()] = py_actionInputMapping;
       }

       py_actionInputsDefinition["InputMappings"] = py_actionInputMappings;

       py_actionInputsDefinitions[actionName.c_str()] = py_actionInputsDefinition;
     }

     return py_actionInputsDefinitions;
   }*/

  std::vector<std::string> getAllAvailableAction() {
      std::vector<std::string> availableAction;
      auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
      for (auto actionInputDefinitionPair : actionInputsDefinitions) {
          auto actionName = actionInputDefinitionPair.first;
          availableAction.push_back(actionName);
      }

      return availableAction;
  };

  std::vector<std::string> getPlayerAvailableAction() {
      std::vector<std::string> availableAction;
      auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
      for (auto actionInputDefinitionPair : actionInputsDefinitions) {
          auto actionName = actionInputDefinitionPair.first;
          auto actionInputDefinition = actionInputDefinitionPair.second;

          auto internal = actionInputDefinition.internal;
          /*"Internal" actions are defined in the environment, but cannot be used by any players
          They can only be spawned from other actions*/
          if (internal) {
              availableAction.push_back(actionName);
          }
      }

      return availableAction;
  };

  std::vector<std::string> getNonPlayerAvailableAction() {
      std::vector<std::string> availableAction;
      auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
      for (auto actionInputDefinitionPair : actionInputsDefinitions) {
          auto actionName = actionInputDefinitionPair.first;
          auto actionInputDefinition = actionInputDefinitionPair.second;

          auto internal = actionInputDefinition.internal;
          /*"Internal" actions are defined in the environment, but cannot be used by any players
          They can only be spawned from other actions*/
          if (!internal) {
              availableAction.push_back(actionName);
          }
      }

      return availableAction;
  };

  std::vector<uint32_t> getInputsIds(std::string actionName) {
      std::vector<uint32_t> inputsIds;
      auto actionInputDefinition = gdyFactory_->findActionInputsDefinition(actionName);

      for (auto inputMapping : actionInputDefinition.inputMappings) {

          auto inputId = inputMapping.first;
          auto actionInputMapping = inputMapping.second;

          inputsIds.push_back(inputId);
      }

      return inputsIds;
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