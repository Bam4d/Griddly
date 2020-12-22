#include "ObjectGenerator.hpp"

#include <spdlog/spdlog.h>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "Object.hpp"

namespace griddly {
ObjectGenerator::ObjectGenerator() {
}

ObjectGenerator::~ObjectGenerator() {
}

void ObjectGenerator::defineNewObject(std::string objectName, uint32_t zIdx, char mapChar, std::unordered_map<std::string, uint32_t> variableDefinitions) {
  spdlog::debug("Defining new object {0}", objectName);

  ObjectDefinition objectDefinition;
  objectDefinition.objectName = objectName;
  objectDefinition.zIdx = zIdx;
  objectDefinition.variableDefinitions = variableDefinitions;

  objectDefinitions_.insert({objectName, std::make_shared<ObjectDefinition>(objectDefinition)});

  objectChars_[mapChar] = objectName;
  objectIds_.insert({objectName, objectCount_++});
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

std::shared_ptr<Object> ObjectGenerator::cloneInstance(std::shared_ptr<Object> toClone, std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables) {
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
  auto id = objectIds_[objectName];
  auto initializedObject = std::shared_ptr<Object>(new Object(objectName, id, playerId, objectZIdx, availableVariables, shared_from_this()));

  if (objectName == avatarObject_) {
    initializedObject->markAsPlayerAvatar();
  }

  for (auto &actionBehaviourDefinition : objectDefinition->actionBehaviourDefinitions) {
    switch (actionBehaviourDefinition.behaviourType) {
      case ActionBehaviourType::SOURCE:

        // Adding the acion preconditions
        for (auto actionPrecondition : actionBehaviourDefinition.actionPreconditions) {
          auto precondition = actionPrecondition.begin();
          initializedObject->addPrecondition(
              actionBehaviourDefinition.actionName,
              actionBehaviourDefinition.destinationObjectName,
              precondition->first,
              precondition->second);
        }

        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case ActionBehaviourType::DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
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

std::shared_ptr<Object> ObjectGenerator::newInstance(std::string objectName, uint32_t playerId, std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables) {
  auto objectDefinition = getObjectDefinition(objectName);

  spdlog::debug("Creating new object {0}.", objectName);

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
  auto id = objectIds_[objectName];
  auto initializedObject = std::shared_ptr<Object>(new Object(objectName, id, playerId, objectZIdx, availableVariables, shared_from_this()));

  if (isAvatar) {
    initializedObject->markAsPlayerAvatar();
  }

  for (auto &actionBehaviourDefinition : objectDefinition->actionBehaviourDefinitions) {
    switch (actionBehaviourDefinition.behaviourType) {
      case ActionBehaviourType::SOURCE:

        // Adding the acion preconditions
        for (auto actionPrecondition : actionBehaviourDefinition.actionPreconditions) {
          auto precondition = actionPrecondition.begin();
          initializedObject->addPrecondition(
              actionBehaviourDefinition.actionName,
              actionBehaviourDefinition.destinationObjectName,
              precondition->first,
              precondition->second);
        }

        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case ActionBehaviourType::DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.sourceObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandArguments,
            actionBehaviourDefinition.conditionalCommands);
        break;
    }
  }

  initializedObject->setInitialActionDefinitions(objectDefinition->initialActionDefinitions);

  return initializedObject;
}  // namespace griddly

void ObjectGenerator::setAvatarObject(std::string objectName) {
  avatarObject_ = objectName;
}

void ObjectGenerator::setActionInputDefinitions(std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions) {
  actionInputsDefinitions_ = actionInputsDefinitions;
}

std::unordered_map<std::string, ActionInputsDefinition> ObjectGenerator::getActionInputDefinitions() const {
  return actionInputsDefinitions_;
}

std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> ObjectGenerator::getObjectDefinitions() const {
  return objectDefinitions_;
}

std::string &ObjectGenerator::getObjectNameFromMapChar(char character) {
  auto objectCharIt = objectChars_.find(character);
  if (objectCharIt == objectChars_.end()) {
    throw std::invalid_argument(fmt::format("Object with map character {0} not defined.", character));
  }
  return objectCharIt->second;
}

std::shared_ptr<ObjectDefinition> &ObjectGenerator::getObjectDefinition(std::string objectName) {
  auto objectDefinitionIt = objectDefinitions_.find(objectName);
  if (objectDefinitionIt == objectDefinitions_.end()) {
    throw std::invalid_argument(fmt::format("Object {0} not defined.", objectName));
  }

  return objectDefinitionIt->second;
}

}  // namespace griddly