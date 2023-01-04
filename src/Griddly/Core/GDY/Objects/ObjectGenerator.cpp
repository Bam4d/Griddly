#include <spdlog/spdlog.h>

#include "ObjectGenerator.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../../Grid.hpp"
#include "Object.hpp"

namespace griddly {
ObjectGenerator::ObjectGenerator() {
  // Define the default _empty object
  ObjectDefinition emptyObjectDefinition;
  emptyObjectDefinition.objectName = "_empty";
  emptyObjectDefinition.zIdx = 0;
  emptyObjectDefinition.variableDefinitions = {};

  objectDefinitions_.insert({"_empty", std::make_shared<ObjectDefinition>(emptyObjectDefinition)});

  // Define the default _boundary object
  ObjectDefinition boundaryObjectDefinition;
  boundaryObjectDefinition.objectName = "_boundary";
  boundaryObjectDefinition.zIdx = 0;
  boundaryObjectDefinition.variableDefinitions = {};
  objectDefinitions_.insert({"_boundary", std::make_shared<ObjectDefinition>(boundaryObjectDefinition)});
}

void ObjectGenerator::defineNewObject(std::string objectName, char mapCharacter, uint32_t zIdx, std::unordered_map<std::string, uint32_t> variableDefinitions) {
  spdlog::debug("Defining new object {0}", objectName);

  ObjectDefinition objectDefinition;
  objectDefinition.objectName = objectName;
  objectDefinition.mapCharacter = mapCharacter;
  objectDefinition.zIdx = zIdx;
  objectDefinition.variableDefinitions = variableDefinitions;

  objectDefinitions_.insert({objectName, std::make_shared<ObjectDefinition>(objectDefinition)});
  objectChars_[mapCharacter] = objectName;
}

void ObjectGenerator::defineActionBehaviour(
    std::string objectName,
    ActionBehaviourDefinition behaviourDefinition) {
  spdlog::debug("Defining object {0} behaviour {1}:{2}", objectName, behaviourDefinition.actionName, behaviourDefinition.commandName);
  auto objectDefinition = getObjectDefinition(objectName);
  objectDefinition->actionBehaviourDefinitions.push_back(behaviourDefinition);
}

void ObjectGenerator::addInitialAction(std::string objectName, std::string actionName, uint32_t actionId, uint32_t delay, bool randomize) {
  spdlog::debug("Defining object {0} initial action {1}", objectName, actionName);
  auto objectDefinition = getObjectDefinition(objectName);
  objectDefinition->initialActionDefinitions.push_back({actionName, actionId, delay, randomize});
}

std::shared_ptr<Object> ObjectGenerator::cloneInstance(std::shared_ptr<Object> toClone, std::shared_ptr<Grid> grid) {
  auto objectName = toClone->getObjectName();
  auto objectDefinition = getObjectDefinition(objectName);
  auto playerId = toClone->getPlayerId();

  spdlog::debug("Cloning player {0} object {1}. {2} variables, {3} behaviours.",
                playerId,
                objectName,
                objectDefinition->variableDefinitions.size(),
                objectDefinition->actionBehaviourDefinitions.size());

  // Initialize the variables for the Object
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables;
  for (auto &variableDefinitions : objectDefinition->variableDefinitions) {
    // Copy the variable from the old object
    auto copiedVariableValue = *toClone->getVariableValue(variableDefinitions.first);

    auto initializedVariable = std::make_shared<int32_t>(copiedVariableValue);
    availableVariables.insert({variableDefinitions.first, initializedVariable});
  }

  auto globalVariables = grid->getGlobalVariables();

  // Initialize global variables
  for (auto &globalVariable : globalVariables) {
    auto variableName = globalVariable.first;
    auto globalVariableInstances = globalVariable.second;

    if (globalVariableInstances.size() == 1) {
      spdlog::debug("Adding reference to global variable {0} to object {1}", variableName, objectName);
      auto instance = globalVariableInstances.at(0);
      availableVariables.insert({variableName, instance});
    } else {
      auto instance = globalVariableInstances.at(playerId);
      spdlog::debug("Adding reference to player variable {0} with value {1} to object {2}", variableName, *instance, objectName);
      availableVariables.insert({variableName, instance});
    }
  }

  auto objectZIdx = objectDefinition->zIdx;
  auto mapCharacter = objectDefinition->mapCharacter;
  auto initializedObject = std::make_shared<Object>(Object(objectName, mapCharacter, playerId, objectZIdx, availableVariables, shared_from_this(), grid));

  if (objectName == avatarObject_) {
    initializedObject->markAsPlayerAvatar();
  }

  initializedObject->setRenderTileId(toClone->getRenderTileId());

  for (auto &actionBehaviourDefinition : objectDefinition->actionBehaviourDefinitions) {
    switch (actionBehaviourDefinition.behaviourType) {
      case ActionBehaviourType::SOURCE:

        if (actionBehaviourDefinition.actionPreconditionsNode.IsDefined()) {
          // Adding the acion preconditions
          initializedObject->addPrecondition(
              actionBehaviourDefinition.actionName,
              actionBehaviourDefinition.behaviourIdx,
              actionBehaviourDefinition.destinationObjectName,
              actionBehaviourDefinition.actionPreconditionsNode);
        }

        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case ActionBehaviourType::DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.sourceObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
    }
  }

  initializedObject->setInitialActionDefinitions(objectDefinition->initialActionDefinitions);

  return initializedObject;
}

const GameStateMapping& ObjectGenerator::getStateMapping() const {
  return gameStateMapping_;
}

const GameObjectData ObjectGenerator::toObjectData(std::shared_ptr<Object> object) const {

}

const std::shared_ptr<Object> ObjectGenerator::fromObjectData(const GameObjectData &objectData, std::shared_ptr<Grid> grid) {
  auto objectName = objectData.name;
  auto objectDefinition = getObjectDefinition(objectName);
  const auto &objectVariableIndexes = objectData.getVariableIndexes(gameStateMapping_);
  auto playerId = objectData.getVariableValue(objectVariableIndexes, "_playerId");

  spdlog::debug("Parsing player {0} object {1}. {2} variables, {3} behaviours.",
                playerId,
                objectName,
                objectDefinition->variableDefinitions.size(),
                objectDefinition->actionBehaviourDefinitions.size());

  // Initialize the variables for the Object
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables;
  for (auto &variableDefinitions : objectDefinition->variableDefinitions) {
    // Copy the variable from the old object
    auto copiedVariableValue = objectData.getVariableValue(objectVariableIndexes, variableDefinitions.first);

    auto initializedVariable = std::make_shared<int32_t>(copiedVariableValue);
    availableVariables.insert({variableDefinitions.first, initializedVariable});
  }

  auto globalVariables = grid->getGlobalVariables();

  // Initialize global variables
  for (auto &globalVariable : globalVariables) {
    auto variableName = globalVariable.first;
    auto globalVariableInstances = globalVariable.second;

    if (globalVariableInstances.size() == 1) {
      spdlog::debug("Adding reference to global variable {0} to object {1}", variableName, objectName);
      auto instance = globalVariableInstances.at(0);
      availableVariables.insert({variableName, instance});
    } else {
      auto instance = globalVariableInstances.at(playerId);
      spdlog::debug("Adding reference to player variable {0} with value {1} to object {2}", variableName, *instance, objectName);
      availableVariables.insert({variableName, instance});
    }
  }

  auto objectZIdx = objectDefinition->zIdx;
  auto mapCharacter = objectDefinition->mapCharacter;
  auto initializedObject = std::make_shared<Object>(Object(objectName, mapCharacter, playerId, objectZIdx, availableVariables, shared_from_this(), grid));

  if (objectName == avatarObject_) {
    initializedObject->markAsPlayerAvatar();
  }

  auto renderTileId = objectData.getVariableValue(objectVariableIndexes, "_renderTileId");
  initializedObject->setRenderTileId(renderTileId);

  for (auto &actionBehaviourDefinition : objectDefinition->actionBehaviourDefinitions) {
    switch (actionBehaviourDefinition.behaviourType) {
      case ActionBehaviourType::SOURCE:

        if (actionBehaviourDefinition.actionPreconditionsNode.IsDefined()) {
          // Adding the acion preconditions
          initializedObject->addPrecondition(
              actionBehaviourDefinition.actionName,
              actionBehaviourDefinition.behaviourIdx,
              actionBehaviourDefinition.destinationObjectName,
              actionBehaviourDefinition.actionPreconditionsNode);
        }

        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case ActionBehaviourType::DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.sourceObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
    }
  }

  initializedObject->setInitialActionDefinitions(objectDefinition->initialActionDefinitions);

  return initializedObject;
}

std::shared_ptr<Object> ObjectGenerator::newInstance(std::string objectName, uint32_t playerId, std::shared_ptr<Grid> grid) {
  spdlog::debug("Creating new object {0}.", objectName);

  auto objectDefinition = getObjectDefinition(objectName);

  auto isAvatar = objectName == avatarObject_;

  // if the object is marked as an avatar, but the playerId is 0 then assume this is the only avatar and set the playerId to 1
  if (playerId == 0 && isAvatar) {
    playerId = 1;
  }

  // Initialize the variables for the Object
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables;
  for (auto &variableDefinitions : objectDefinition->variableDefinitions) {
    auto variableName = variableDefinitions.first;
    auto initializedVariable = std::make_shared<int32_t>(variableDefinitions.second);
    spdlog::debug("Creating local variable {0} with value {1} for object {2}", variableName, *initializedVariable, objectName);
    availableVariables.insert({variableDefinitions.first, initializedVariable});
  }

  auto globalVariables = grid->getGlobalVariables();

  // Initialize global variables
  for (auto &globalVariable : globalVariables) {
    auto variableName = globalVariable.first;
    auto globalVariableInstances = globalVariable.second;

    spdlog::debug("Adding reference to global variable {0} to object {1}", variableName, objectName);
    if (globalVariableInstances.size() == 1) {
      auto instance = globalVariableInstances.at(0);
      availableVariables.insert({variableName, instance});
    } else {
      auto instance = globalVariableInstances.at(playerId);
      availableVariables.insert({variableName, instance});
    }
  }

  auto objectZIdx = objectDefinition->zIdx;
  auto mapCharacter = objectDefinition->mapCharacter;
  auto initializedObject = std::make_shared<Object>(Object(objectName, mapCharacter, playerId, objectZIdx, availableVariables, shared_from_this(), grid));

  if (isAvatar) {
    initializedObject->markAsPlayerAvatar();
  }

  for (auto &actionBehaviourDefinition : objectDefinition->actionBehaviourDefinitions) {
    switch (actionBehaviourDefinition.behaviourType) {
      case ActionBehaviourType::SOURCE:

        if (actionBehaviourDefinition.actionPreconditionsNode.IsDefined()) {
          // Adding the acion preconditions
          initializedObject->addPrecondition(
              actionBehaviourDefinition.actionName,
              actionBehaviourDefinition.behaviourIdx,
              actionBehaviourDefinition.destinationObjectName,
              actionBehaviourDefinition.actionPreconditionsNode);
        }

        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case ActionBehaviourType::DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.behaviourIdx,
            actionBehaviourDefinition.sourceObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
    }
  }

  initializedObject->setInitialActionDefinitions(objectDefinition->initialActionDefinitions);

  return initializedObject;
}

void ObjectGenerator::setAvatarObject(std::string objectName) {
  avatarObject_ = objectName;
}

void ObjectGenerator::setActionInputDefinitions(std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions) {
  actionInputsDefinitions_ = actionInputsDefinitions;
}

void ObjectGenerator::setActionTriggerDefinitions(std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions) {
  actionTriggerDefinitions_ = actionTriggerDefinitions;
}

void ObjectGenerator::setBehaviourProbabilities(std::unordered_map<std::string, std::vector<float>> behaviourProbabilities) {
  behaviourProbabilities_ = behaviourProbabilities;
}

const std::unordered_map<std::string, ActionInputsDefinition> &ObjectGenerator::getActionInputDefinitions() const {
  return actionInputsDefinitions_;
}

const std::unordered_map<std::string, ActionTriggerDefinition> &ObjectGenerator::getActionTriggerDefinitions() const {
  return actionTriggerDefinitions_;
}

const std::unordered_map<std::string, std::vector<float>> &ObjectGenerator::getBehaviourProbabilities() const {
  return behaviourProbabilities_;
}

// The order of object definitions needs to be consistent across levels and maps, so we have to make sure this is ordered here.
const std::map<std::string, std::shared_ptr<ObjectDefinition>> &ObjectGenerator::getObjectDefinitions() const {
  return objectDefinitions_;
}

std::string &ObjectGenerator::getObjectNameFromMapChar(char character) {
  auto objectCharIt = objectChars_.find(character);
  if (objectCharIt == objectChars_.end()) {
    throw std::invalid_argument(fmt::format("Object with map character {0} not defined.", character));
  }
  return objectCharIt->second;
}

const std::shared_ptr<ObjectDefinition> &ObjectGenerator::getObjectDefinition(const std::string &objectName) const {
  auto objectDefinitionIt = objectDefinitions_.find(objectName);
  if (objectDefinitionIt == objectDefinitions_.end()) {
    throw std::invalid_argument(fmt::format("Object {0} not defined.", objectName));
  }

  return objectDefinitionIt->second;
}

}  // namespace griddly