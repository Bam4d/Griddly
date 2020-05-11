#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "Object.hpp"

namespace griddy {

enum class ActionBehaviourType {
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

struct ObjectDefinition {
  std::string objectName;
  std::unordered_map<std::string, uint32_t> parameterDefinitions;
  std::vector<ActionBehaviourDefinition> actionBehaviourDefinitions;
  uint32_t zIdx;
};

class ObjectGenerator {
 public:
  ObjectGenerator();

  ~ObjectGenerator();

  virtual void defineNewObject(std::string objectName, uint32_t zIdx, char mapChar, std::unordered_map<std::string, uint32_t> parameterDefinitions);
  virtual void defineActionBehaviour(std::string objectName, ActionBehaviourDefinition behaviourDefinition);
  virtual void defineGlobalParameter(std::string parameterName, std::shared_ptr<int32_t> initialValue);

  virtual std::shared_ptr<Object> newInstance(std::string objectName);

  virtual std::string& getObjectNameFromMapChar(char character);

 private:
  std::unordered_map<char, std::string> objectChars_;
  std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> objectDefinitions_;
  std::unordered_map<std::string, uint32_t> objectIds_;
  uint32_t objectCount_ = 0;

  std::unordered_map<std::string, std::shared_ptr<int32_t>> globalParameters_;

  
  std::shared_ptr<ObjectDefinition>& getObjectDefinition(std::string objectName);
};
}  // namespace griddy