#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "Object.hpp"

namespace griddy {

enum ActionBehaviourType {
    SOURCE,
    DESTINATION
};

struct ActionBehaviourDefinition {
  ActionBehaviourType behaviourType;
  std::string sourceObjectName;
  std::string destinationObjectName;
  std::string actionName;
  std::string commandName;
  std::vector<std::string> commandParameters;
  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands;
};

class ObjectDefinition {
 public:
  ObjectDefinition(std::string objectName, std::unordered_map<std::string, uint> parameterDefinitions)
      : objectName_(objectName), 
      parameterDefinitions_(parameterDefinitions), 
      actionBehaviourDefinitions_(std::shared_ptr<std::vector<ActionBehaviourDefinition>>(new std::vector<ActionBehaviourDefinition>())) {
  }

  const std::string objectName_;
  const std::unordered_map<std::string, uint> parameterDefinitions_;
  const std::shared_ptr<std::vector<ActionBehaviourDefinition>> actionBehaviourDefinitions_;
};

class ObjectGenerator {
 public:
  ObjectGenerator();

  ~ObjectGenerator();

  void defineNewObject(std::string objectName, char mapChar, std::unordered_map<std::string, uint> parameterDefinitions);
  void defineActionBehaviour(std::string objectName, ActionBehaviourDefinition behaviourDefinition);

  std::shared_ptr<Object> newInstance(std::string objectName);

  std::string& getObjectNameFromMapChar(char character);

 private:
  std::unordered_map<char, std::string> objectChars_;
  std::unordered_map<std::string, ObjectDefinition> objectDefinitions_;
  std::unordered_map<std::string, uint> objectIds_;
  uint objectCount_ = 0;

  
  ObjectDefinition &getObjectDefinition(std::string objectName);
};
}  // namespace griddy