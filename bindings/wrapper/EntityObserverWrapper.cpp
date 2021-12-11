#pragma once

#include <spdlog/spdlog.h>

#include "../../src/Griddly/Core/TurnBasedGameProcess.hpp"
#include "NumpyWrapper.cpp"
#include "StepPlayerWrapper.cpp"

namespace griddly {

struct EntityObservations {
  std::map<std::string, std::vector<std::vector<float>>> entityObservation{};
  std::map<size_t, uint32_t> entityIdMap{};
  std::vector<size_t> entityIds{};
};

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
    auto actionsAndMasks = buildEntityMasks(playerId, entityObservationsAndIds.entityIdMap);

    observation["Entities"] = entityObservationsAndIds.entityObservation;
    observation["EntityIds"] = entityObservationsAndIds.entityIds;
    observation["EntityIdMap"] = entityObservationsAndIds.entityIdMap;
    observation["EntityMasks"] = actionsAndMasks;

    return observation;
  }

 private:
  // Build entity masks (for transformer architectures)
  py::dict buildEntityMasks(int playerId, std::map<size_t, uint32_t> entityIdMap) const {
    std::map<std::string, std::vector<std::vector<uint32_t>>> entityMasks;
    std::map<std::string, std::vector<uint32_t>> actorIdx;

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
        std::vector<uint32_t> mask(actionInputsDefinitions[actionName].inputMappings.size() + 1);
        mask[0] = 1;  // NOP is always available

        auto objectAtLocation = grid->getObject(location);
        auto entityId = std::hash<std::shared_ptr<Object>>()(objectAtLocation);
        auto actionIdsForName = gameProcess_->getAvailableActionIdsAtLocation(locationVec, actionName);

        for (auto id : actionIdsForName) {
          mask[id] = 1;
        }

        entityMasks[actionName].push_back(mask);
        actorIdx[actionName].push_back(entityIdMap.at(entityId));

        allAvailableActionNames.insert(actionName);
      }
    }

    for (auto actionName : allAvailableActionNames) {
      py::dict entitiesAndMasksForAction;
      entitiesAndMasksForAction["ActorIdx"] = actorIdx[actionName];
      entitiesAndMasksForAction["Masks"] = entityMasks[actionName];

      entitiesAndMasks[actionName.c_str()] = entitiesAndMasksForAction;
    }

    return entitiesAndMasks;
  }

  // Build entity observations (for transformer architectures)
  EntityObservations buildEntityObservations(int playerId) const {
    EntityObservations entityObservationsAndIds;

    std::map<std::string, std::vector<std::shared_ptr<Object>>> entityObjects;
    std::map<std::string, std::vector<std::vector<float>>> entityObservations;
    std::map<size_t, uint32_t> entityIdMap;
    std::vector<size_t> entityIds;

    auto grid = gameProcess_->getGrid();

    for (auto object : grid->getObjects()) {
      auto name = object->getObjectName();
      auto location = object->getLocation();
      auto orientationRadians = object->getObjectOrientation().getAngleRadians();
      auto objectPlayerId = object->getPlayerId();
      auto zIdx = object->getZIdx();

      auto featureVariables = entityVariableMapping_.at(name);

      auto numVariables = featureVariables.size();
      auto numFeatures = 5 + numVariables;

      std::vector<float> featureVector(numFeatures);
      featureVector[0] = static_cast<float>(location[0]);
      featureVector[1] = static_cast<float>(location[1]);
      featureVector[2] = static_cast<float>(zIdx);
      featureVector[3] = static_cast<float>(orientationRadians);
      featureVector[4] = static_cast<float>(objectPlayerId);
      for (int32_t i = 0; i < numVariables; i++) {
        auto variableValue = *object->getVariableValue(featureVariables[i]);
        featureVector[5 + i] = static_cast<float>(variableValue);
      }

      entityObservations[name].push_back(featureVector);
      entityObjects[name].push_back(object);
    }

    // All entities are in the map and now we need to calculate the entity ids in the same order
    for (auto objects : entityObjects) {
      for (auto object : objects.second) {
        auto hash = std::hash<std::shared_ptr<Object>>()(object);
        entityIdMap.insert({hash, entityIds.size()});
        entityIds.push_back(hash);
      }
    }

    return {
      entityObservations,
      entityIdMap,
      entityIds
    };
  }

  std::unordered_map<std::string, std::vector<std::string>> entityVariableMapping_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;
};
}  // namespace griddly