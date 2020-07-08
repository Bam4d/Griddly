#pragma once
#include <yaml-cpp/yaml.h>

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Actions/Direction.hpp"

#define BehaviourCommandArguments std::unordered_map<std::string, YAML::Node>
#define BehaviourFunction std::function<BehaviourResult(std::shared_ptr<Action>)>
#define PreconditionFunction std::function<bool()>

namespace griddly {

class Grid;
class Action;
class ObjectGenerator;
class InputMapping;

struct InitialActionDefinition {
  std::string actionName;
  uint32_t actionId;
  uint32_t delay;
  bool randomize;
};

struct SingleInputMapping {
  glm::ivec2 vectorToDest{};
  glm::ivec2 orientationVector{};
  uint32_t actionId;
  bool relative;
  bool internal;
};

struct BehaviourResult {
  bool abortAction = false;
  int32_t reward = 0;
};

class Object : public std::enable_shared_from_this<Object> {
 public:
  virtual glm::ivec2 getLocation() const;

  // playerId of 0 means the object does not belong to any player in particular, (walls etc)
  virtual void init(uint32_t playerId, glm::ivec2 location, std::shared_ptr<Grid> grid_);

  virtual void init(uint32_t playerId, glm::ivec2 location, DiscreteOrientation orientation, std::shared_ptr<Grid> grid);

  virtual std::string getObjectName() const;

  virtual uint32_t getObjectId() const;

  virtual std::string getDescription() const;

  virtual uint32_t getPlayerId() const;

  virtual uint32_t getZIdx() const;

  virtual DiscreteOrientation getObjectOrientation() const;

  virtual bool isPlayerAvatar() const;
  virtual void markAsPlayerAvatar();  // Set this object as a player avatar

  virtual bool checkPreconditions(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) const;

  virtual void addPrecondition(std::string actionName, std::string destinationObjectName, std::string commandName, BehaviourCommandArguments commandArguments);

  virtual BehaviourResult onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action);

  virtual BehaviourResult onActionDst(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action);

  virtual void addActionSrcBehaviour(std::string action, std::string destinationObjectName, std::string commandName, BehaviourCommandArguments commandArguments, std::unordered_map<std::string, BehaviourCommandArguments> nestedCommands);

  virtual void addActionDstBehaviour(std::string action, std::string sourceObjectName, std::string commandName, BehaviourCommandArguments commandArguments, std::unordered_map<std::string, BehaviourCommandArguments> nestedCommands);

  virtual std::shared_ptr<int32_t> getVariableValue(std::string variableName);

  // Initial actions for objects
  virtual std::vector<std::shared_ptr<Action>> getInitialActions();
  virtual void setInitialActionDefinitions(std::vector<InitialActionDefinition> actionDefinitions);

  Object(std::string objectName, uint32_t id, uint32_t zIdx, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator);

  ~Object();

 private:
  // Have to be shared pointers because they are used as variables
  std::shared_ptr<int32_t> x_ = std::make_shared<int32_t>(0);
  std::shared_ptr<int32_t> y_ = std::make_shared<int32_t>(0);

  DiscreteOrientation orientation_ = DiscreteOrientation(Direction::NONE);

  uint32_t playerId_;
  const std::string objectName_;
  const uint32_t id_;
  const uint32_t zIdx_;
  bool isPlayerAvatar_ = false;

  std::vector<InitialActionDefinition> initialActionDefinitions_;

  // action -> destination -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<BehaviourFunction>>> srcBehaviours_;

  // action -> source -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<BehaviourFunction>>> dstBehaviours_;

  // action -> destination -> [precondition list]
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PreconditionFunction>>> actionPreconditions_;

  // The variables that are available in the object for behaviour commands to interact with
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables_;

  std::shared_ptr<Grid> grid_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  virtual bool moveObject(glm::ivec2 newLocation);

  virtual void removeObject();

  SingleInputMapping getInputMapping(std::string actionName, uint32_t actionId, bool randomize, InputMapping fallback);

  std::unordered_map<std::string, std::shared_ptr<int32_t>> resolveVariables(BehaviourCommandArguments variables);

  PreconditionFunction instantiatePrecondition(std::string commandName, BehaviourCommandArguments commandArguments);
  BehaviourFunction instantiateBehaviour(std::string commandName, BehaviourCommandArguments commandArguments);
  BehaviourFunction instantiateConditionalBehaviour(std::string commandName, BehaviourCommandArguments commandArguments, std::unordered_map<std::string, BehaviourCommandArguments> subCommands);

  std::string getStringMapValue(std::unordered_map<std::string, std::string> map, std::string mapKey);
};

}  // namespace griddly