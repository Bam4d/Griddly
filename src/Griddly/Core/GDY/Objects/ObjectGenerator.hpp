#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../GameState.hpp"
#include "../Actions/Action.hpp"
#include "Object.hpp"

namespace griddly {

class Grid;

enum class ActionBehaviourType {
  SOURCE,
  DESTINATION
};

struct ActionBehaviourDefinition {
  ActionBehaviourType behaviourType;
  uint32_t behaviourIdx;
  std::string sourceObjectName;
  std::string destinationObjectName;
  std::string actionName;
  std::string commandName;
  CommandArguments commandArguments;
  YAML::Node actionPreconditionsNode = YAML::Node(YAML::NodeType::Undefined);
  CommandList conditionalCommands{};
  float executionProbability = 1.0;
};

struct ObjectDefinition {
  std::string objectName;
  char mapCharacter;
  std::unordered_map<std::string, uint32_t> variableDefinitions{};
  std::vector<ActionBehaviourDefinition> actionBehaviourDefinitions{};
  std::vector<InitialActionDefinition> initialActionDefinitions{};
  uint32_t zIdx = 0;
};

class ObjectGenerator : public std::enable_shared_from_this<ObjectGenerator> {
 public:
  ObjectGenerator();

  virtual ~ObjectGenerator() = default;

  virtual void defineNewObject(std::string objectName, char mapCharacter, uint32_t zIdx, std::unordered_map<std::string, uint32_t> variableDefinitions);
  virtual void defineActionBehaviour(std::string objectName, ActionBehaviourDefinition behaviourDefinition);
  virtual void addInitialAction(std::string objectName, std::string actionName, uint32_t actionId, uint32_t delay, bool randomize = false);

  virtual std::shared_ptr<Object> newInstance(std::string objectName, uint32_t playerId, std::shared_ptr<Grid> grid);
  virtual std::shared_ptr<Object> cloneInstance(std::shared_ptr<Object> toClone, std::shared_ptr<Grid> grid);

  virtual std::string& getObjectNameFromMapChar(char character);

  virtual void setAvatarObject(std::string objectName);

  virtual void setActionInputDefinitions(std::unordered_map<std::string, ActionInputsDefinition> actionInputDefinitions);
  virtual void setActionTriggerDefinitions(std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions);
  virtual void setBehaviourProbabilities(std::unordered_map<std::string, std::vector<float>> behaviourProbabilities);
  virtual void defineGlobalVariables(std::map<std::string, GlobalVariableDefinition> globalVariableDefinitions);

  virtual const std::unordered_map<std::string, ActionInputsDefinition>& getActionInputDefinitions() const;
  virtual const std::unordered_map<std::string, ActionTriggerDefinition>& getActionTriggerDefinitions() const;
  virtual const std::unordered_map<std::string, std::vector<float>>& getBehaviourProbabilities() const;

  virtual const std::map<std::string, std::shared_ptr<ObjectDefinition>>& getObjectDefinitions() const;

  virtual const GameStateMapping& getStateMapping() const;
  virtual const GameObjectData toObjectData(std::shared_ptr<Object> object) const;
  virtual const std::shared_ptr<Object> fromObjectData(const GameObjectData& objectData, std::shared_ptr<Grid> grid);

 private:
  std::unordered_map<char, std::string> objectChars_;

  // This needs to be ordered, so object types are always in a consistent order across multiple instantiations of games.
  std::map<std::string, std::shared_ptr<ObjectDefinition>> objectDefinitions_;
  std::map<std::string, GlobalVariableDefinition> globalVariableDefinitions_;

  std::string avatarObject_;

  // TODO: These should all be merged into one object probably
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions_;
  std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions_;
  std::unordered_map<std::string, std::vector<float>> behaviourProbabilities_;

  const std::shared_ptr<ObjectDefinition>& getObjectDefinition(const std::string& objectName) const;

  // Maps variable indexes in game states
  GameStateMapping gameStateMapping_;

  std::shared_ptr<RandomGenerator> randomGenerator_ = std::make_shared<RandomGenerator>(RandomGenerator());
};
}  // namespace griddly