#pragma once

#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"

namespace griddly {

class Py_EntityObserverWrapper {
 public:
  Py_EntityObserverWrapper(py::dict entityObserverConfig, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess) : gameProcess_(gameProcess), gdyFactory_(gdyFactory) {
    spdlog::debug("Created entity observer.");

    if (entityObserverConfig.contains("VariableMapping")) {
      entityVariableMapping_ = entityObserverConfig["variableMapping"].cast<std::unordered_map<std::string, std::vector<std::string>>>();
    } else {
      entityVariableMapping_ = gameProcess_->getGrid()->getObjectVariableMap();
    }

    for (auto entityVariables : entityVariableMapping_) {
      for (auto variableName : entityVariables.second) {
        spdlog::debug("Entity {0}, will include variable {1} in entity observations.", entityVariables.first, variableName);
      }
    }
  }

  py::dict observe(int playerId) {
    py::dict observation;

    auto entityObservationsAndIds = buildEntityObservations(playerId);
    auto actionsAndMasks = buildEntityMasks(playerId);

    observation["Entities"] = entityObservationsAndIds["Entities"];
    observation["EntityIds"] = entityObservationsAndIds["EntityIds"];
    observation["EntityMasks"] = actionsAndMasks;

    return observation;
  }

 private:
  // Build entity masks (for transformer architectures)
  py::dict buildEntityMasks(int playerId) const {
    std::map<std::string, std::vector<std::vector<int>>> entityMasks;
    std::map<std::string, std::vector<size_t>> entityIds;

    std::unordered_set<std::string> allAvailableActionNames;

    py::dict entitiesAndMasks;

    auto grid = gameProcess_->getGrid();

    for (auto actionNamesAtLocation : gameProcess_->getAvailableActionNames(playerId)) {
      auto location = actionNamesAtLocation.first;
      auto actionNames = actionNamesAtLocation.second;

      auto locationVec = glm::ivec2{location[0], location[1]};

      for (auto actionName : actionNames) {
        spdlog::debug("[{0}] available at location [{1}, {2}]", actionName, location.x, location.y);

        auto actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
        std::vector<int> mask(actionInputsDefinitions[actionName].inputMappings.size() + 1);
        mask[0] = 1;  // NOP is always available

        auto objectAtLocation = grid->getObject(location);
        auto entityId = std::hash<std::shared_ptr<Object>>()(objectAtLocation);
        auto actionIdsForName = gameProcess_->getAvailableActionIdsAtLocation(locationVec, actionName);

        for (auto id : actionIdsForName) {
          mask[id] = 1;
        }

        entityMasks[actionName].push_back(mask);
        entityIds[actionName].push_back(entityId);

        allAvailableActionNames.insert(actionName);
      }
    }

    for (auto actionName : allAvailableActionNames) {
      py::dict entitiesAndMasksForAction;
      entitiesAndMasksForAction["EntityIds"] = entityIds[actionName];
      entitiesAndMasksForAction["Masks"] = entityMasks[actionName];

      entitiesAndMasks[actionName.c_str()] = entitiesAndMasksForAction;
    }

    return entitiesAndMasks;
  }

  // Build entity observations (for transformer architectures)
  py::dict buildEntityObservations(int playerId) const {
    py::dict entityObservationsAndIds;

    std::map<std::string, std::vector<std::vector<float>>> entityObservations;
    std::vector<size_t> entityIds;

    auto grid = gameProcess_->getGrid();

    for (auto object : grid->getObjects()) {
      auto name = object->getObjectName();
      auto location = object->getLocation();
      auto orientation = object->getObjectOrientation().getUnitVector();
      auto objectPlayerId = object->getPlayerId();
      auto zIdx = object->getZIdx();

      auto featureVariables = entityVariableMapping_.at(name);

      auto numVariables = featureVariables.size();
      auto numFeatures = 5 + numVariables;

      std::vector<float> featureVector(numFeatures);
      featureVector[0] = static_cast<float>(location[0]);
      featureVector[1] = static_cast<float>(location[1]);
      featureVector[2] = static_cast<float>(zIdx);
      featureVector[3] = static_cast<float>(orientation[0] + 2 * orientation[1]);
      featureVector[4] = static_cast<float>(objectPlayerId);
      for (int32_t i = 0; i < numVariables; i++) {
        auto variableValue = *object->getVariableValue(featureVariables[i]);
        featureVector[5 + i] = static_cast<float>(variableValue);
      }

      entityObservations[name].push_back(featureVector);

      entityIds.push_back(std::hash<std::shared_ptr<Object>>()(object));
    }

    entityObservationsAndIds["Entities"] = entityObservations;
    entityObservationsAndIds["EntityIds"] = entityIds;

    return entityObservationsAndIds;
  }

  std::unordered_map<std::string, std::vector<std::string>> entityVariableMapping_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;
};
}  // namespace griddly