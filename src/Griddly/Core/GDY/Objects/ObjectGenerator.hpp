#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "Object.hpp"
#include "../Actions/Action.hpp"

namespace griddly {

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
  BehaviourCommandArguments commandArguments;
  std::vector<std::unordered_map<std::string, BehaviourCommandArguments>> actionPreconditions;
  std::unordered_map<std::string, BehaviourCommandArguments> conditionalCommands;
};

struct ObjectDefinition {
  std::string objectName;
  std::unordered_map<std::string, uint32_t> variableDefinitions;
  std::vector<ActionBehaviourDefinition> actionBehaviourDefinitions;
  std::vector<InitialActionDefinition> initialActionDefinitions;
  uint32_t zIdx;
};

class ObjectGenerator : public std::enable_shared_from_this<ObjectGenerator> {
 public:
  ObjectGenerator();

  ~ObjectGenerator();

  virtual void defineNewObject(std::string objectName, uint32_t zIdx, char mapChar, std::unordered_map<std::string, uint32_t> variableDefinitions);
  virtual void setAvatarObject(std::string objectName);
  virtual void defineActionBehaviour(std::string objectName, ActionBehaviourDefinition behaviourDefinition);
  virtual void addInitialAction(std::string objectName, std::string actionName, uint32_t actionId, uint32_t delay, bool randomize=false);

  virtual std::shared_ptr<Object> newInstance(std::string objectName, std::unordered_map<std::string, std::shared_ptr<int32_t>> globalVariables);

  virtual std::string& getObjectNameFromMapChar(char character);

  virtual void setActionInputDefinitions(std::unordered_map<std::string, ActionInputsDefinition> actionInputDefinitions);
  virtual std::unordered_map<std::string, ActionInputsDefinition> getActionInputDefinitions() const;

 private:
  std::unordered_map<char, std::string> objectChars_;
  std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> objectDefinitions_;
  std::unordered_map<std::string, uint32_t> objectIds_;
  uint32_t objectCount_ = 0;

  std::string avatarObject_;
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions_;

  std::shared_ptr<ObjectDefinition>& getObjectDefinition(std::string objectName);
};
}  // namespace griddly