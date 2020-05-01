#include "ObjectGenerator.hpp"

#include <spdlog/spdlog.h>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "Object.hpp"

namespace griddy {
ObjectGenerator::ObjectGenerator() {
}

ObjectGenerator::~ObjectGenerator() {
}

void ObjectGenerator::defineNewObject(std::string objectName, char mapChar, std::unordered_map<std::string, uint32_t> parameterDefinitions) {
  spdlog::debug("Defining new object {0}", objectName);
  objectDefinitions_.insert({objectName, ObjectDefinition(objectName, parameterDefinitions)});

  objectChars_[mapChar] = objectName;
  objectIds_.insert({objectName, objectCount_++});
}

void ObjectGenerator::defineActionBehaviour(
    std::string objectName,
    ActionBehaviourDefinition behaviourDefinition) {
  auto objectDefinition = getObjectDefinition(objectName);
  objectDefinition.actionBehaviourDefinitions_->push_back(behaviourDefinition);

  spdlog::debug("{0} behaviours {1}", objectName, objectDefinition.actionBehaviourDefinitions_->size());
}

std::shared_ptr<Object> ObjectGenerator::newInstance(std::string objectName) {
  auto objectDefinition = getObjectDefinition(objectName);

  spdlog::debug("Creating new object {0}. {1} parameters, {2} behaviours.",
                objectName, 
                objectDefinition.parameterDefinitions_.size(), 
                objectDefinition.actionBehaviourDefinitions_->size());

  // Initialize the parameters for the Object
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters;
  for (auto &parameterDefinitions : objectDefinition.parameterDefinitions_) {
    auto initializedParameter = std::shared_ptr<int32_t>(new int32_t(parameterDefinitions.second));
    availableParameters.insert({parameterDefinitions.first, initializedParameter});
  }

  auto id = objectIds_[objectName];
  auto initializedObject = std::shared_ptr<Object>(new Object(objectName, id, availableParameters));

  for (auto &actionBehaviourDefinition : *objectDefinition.actionBehaviourDefinitions_) {
    switch (actionBehaviourDefinition.behaviourType) {
      case SOURCE:
        initializedObject->addActionSrcBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.destinationObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandParameters,
            actionBehaviourDefinition.conditionalCommands);
        break;
      case DESTINATION:
        initializedObject->addActionDstBehaviour(
            actionBehaviourDefinition.actionName,
            actionBehaviourDefinition.sourceObjectName,
            actionBehaviourDefinition.commandName,
            actionBehaviourDefinition.commandParameters,
            actionBehaviourDefinition.conditionalCommands);
        break;
    }
  }

  return initializedObject;
}

std::string& ObjectGenerator::getObjectNameFromMapChar(char character) {
  return objectChars_[character];
}

ObjectDefinition &ObjectGenerator::getObjectDefinition(std::string objectName) {
  auto objectDefinitionIt = objectDefinitions_.find(objectName);
  if (objectDefinitionIt == objectDefinitions_.end()) {
    throw std::invalid_argument(fmt::format("Object {0} not defined.", objectName));
  }

  return objectDefinitionIt->second;
}

}  // namespace griddy