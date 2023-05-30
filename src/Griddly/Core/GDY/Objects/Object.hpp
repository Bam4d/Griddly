#pragma once
#include <yaml-cpp/yaml.h>

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../Actions/Direction.hpp"
#include "../ConditionResolver.hpp"
// #include "../../AStarPathFinder.hpp"
#include "../YAMLUtils.hpp"
#include "ObjectVariable.hpp"

#define CommandArguments std::map<std::string, YAML::Node>
#define BehaviourFunction std::function<BehaviourResult(const std::shared_ptr<Action>&)>
#define BehaviourCondition std::function<bool(const std::shared_ptr<Action>&)>
#define CommandList std::vector<std::pair<std::string, CommandArguments>>

namespace griddly {

class Grid;
class Action;
class ObjectGenerator;
class InputMapping;
class PathFinder;
enum class PathFinderMode;
class CollisionDetector;

struct InitialActionDefinition {
  std::string actionName;
  uint32_t actionId = 0;
  uint32_t delay = 0;
  bool randomize = false;
  float executionProbability = 1.0;
};

struct SingleInputMapping {
  bool relative = false;
  bool internal = false;
  bool mappedToGrid = false;

  // if the action is relative to a source object
  glm::ivec2 vectorToDest{};
  glm::ivec2 orientationVector{};
  uint32_t actionId = 0;

  // If the action can be perform in any grid location
  glm::ivec2 destinationLocation{};

  // Action metadata
  std::unordered_map<std::string, int32_t> metaData{};
};

struct BehaviourResult {
  bool abortAction = false;
  std::unordered_map<uint32_t, int32_t> rewards{};
};

enum class ActionExecutor {
  ACTION_PLAYER_ID,
  OBJECT_PLAYER_ID,
};

struct PathFinderConfig {
  std::shared_ptr<PathFinder> pathFinder = nullptr;
  std::shared_ptr<CollisionDetector> collisionDetector = nullptr;
  glm::ivec2 endLocation{0, 0};
  PathFinderMode mode;
  uint32_t maxSearchDepth = 100;
};

class Object : public std::enable_shared_from_this<Object>, ConditionResolver<BehaviourCondition> {
 public:
  virtual const glm::ivec2& getLocation() const;

  virtual void init(glm::ivec2 location);

  virtual void init(glm::ivec2 location, DiscreteOrientation orientation);

  virtual const std::string& getObjectName() const;

  virtual const int32_t getId() const;

  virtual char getMapCharacter() const;

  virtual const std::string& getObjectRenderTileName() const;

  virtual std::string getDescription() const;

  virtual uint32_t getPlayerId() const;

  virtual int32_t getZIdx() const;

  virtual DiscreteOrientation getObjectOrientation() const;

  virtual bool isPlayerAvatar() const;

  virtual bool isRemoved() const;

  virtual void setRenderTileId(uint32_t renderTileId);

  virtual uint32_t getRenderTileId() const;

  virtual void markAsPlayerAvatar();  // Set this object as a player avatar

  virtual bool isValidAction(std::shared_ptr<Action> action) const;

  virtual std::vector<uint32_t> getValidBehaviourIdxs(std::shared_ptr<Action> action) const;

  virtual void addPrecondition(const std::string& actionName, uint32_t behaviourIdx, const std::string& destinationObjectName, YAML::Node& conditionsNode);

  virtual BehaviourResult onActionSrc(std::string destinationObjectName, std::shared_ptr<Action> action, std::vector<uint32_t> behaviourIdxs);

  virtual BehaviourResult onActionDst(std::shared_ptr<Action> action, std::vector<uint32_t> behaviourIdxs);

  virtual void addActionSrcBehaviour(const std::string& action, uint32_t behaviourIdx, const std::string& destinationObjectName, const std::string& commandName, CommandArguments commandArguments, CommandList nestedCommands);

  virtual void addActionDstBehaviour(const std::string& action, uint32_t behaviourIdx, const std::string& sourceObjectName, const std::string& commandName, CommandArguments commandArguments, CommandList nestedCommands);

  virtual std::shared_ptr<int32_t> getVariableValue(std::string variableName);

  virtual std::unordered_map<std::string, std::shared_ptr<int32_t>> getAvailableVariables() const;

  virtual std::unordered_set<std::string> getAvailableActionNames() const;

  // Initial actions for objects
  virtual std::vector<std::shared_ptr<Action>> getInitialActions(std::shared_ptr<Action> originatingAction);
  virtual void setInitialActionDefinitions(std::vector<InitialActionDefinition> actionDefinitions);

  // Conditional functions
  BehaviourResult executeBehaviourFunctionList(std::unordered_map<uint32_t, int32_t>& rewardAccumulator, const std::vector<BehaviourFunction>& behaviourList, const std::shared_ptr<Action>& action) const;

  Object(const int32_t id, const std::string& objectName, char mapCharacter, uint32_t playerId, uint32_t zIdx, const std::unordered_map<std::string, std::shared_ptr<int32_t>>& availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator, std::weak_ptr<Grid> grid);
  
  virtual ~Object();

 private:

  // Have to be shared pointers because they are used as variables
  std::shared_ptr<int32_t> x_ = std::make_shared<int32_t>(0);
  std::shared_ptr<int32_t> y_ = std::make_shared<int32_t>(0);

  glm::ivec2 location_;

  DiscreteOrientation orientation_;

  std::shared_ptr<int32_t> playerId_ = std::make_shared<int32_t>(0);
  std::shared_ptr<int32_t> renderTileId_ = std::make_shared<int32_t>(0);
  const std::string objectName_;
  const int32_t id_;
  const char mapCharacter_;
  const int32_t zIdx_;
  std::string renderTileName_;
  bool isPlayerAvatar_ = false;

  std::vector<InitialActionDefinition> initialActionDefinitions_;

  // action -> destination -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<BehaviourFunction>>>> srcBehaviours_;

  // action -> source -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<BehaviourFunction>>>> dstBehaviours_;

  // action -> destination -> [precondition list]
  std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<uint32_t, BehaviourCondition>>> actionPreconditions_;

  // The variables that are available in the object for behaviour commands to interact with
  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables_;

  std::shared_ptr<Grid> grid() const;
  const std::weak_ptr<Grid> grid_;

  std::unordered_set<std::string> availableActionNames_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  virtual bool moveObject(glm::ivec2 newLocation);

  virtual void removeObject();
  bool removed_ = false;

  SingleInputMapping getInputMapping(const std::string& actionName, uint32_t actionId, bool randomize, InputMapping fallback);

  PathFinderConfig configurePathFinder(YAML::Node& searchNode, std::string actionName);

  template <typename C>
  static C getCommandArgument(CommandArguments& commandArguments, std::string commandArgumentKey, C defaultValue);

  std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> resolveActionMetaData(CommandArguments& commandArguments);

  std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> resolveVariables(CommandArguments& variables, bool allowStrings = false) const;

  BehaviourCondition resolveConditionArguments(const std::function<bool(int32_t, int32_t)> conditionFunction, YAML::Node& conditionArgumentsNode) const override;
  BehaviourCondition resolveAND(const std::vector<BehaviourCondition>& conditionList) const override;
  BehaviourCondition resolveOR(const std::vector<BehaviourCondition>& conditionList) const override;

  BehaviourFunction instantiateBehaviour(const std::string& commandName, CommandArguments& commandArguments);
  BehaviourFunction instantiateConditionalBehaviour(const std::string& commandName, CommandArguments& commandArguments, CommandList& subCommands);

  ActionExecutor getActionExecutorFromString(const std::string& executorString) const;
  PathFinderMode getPathFinderModeFromString(const std::string& modeString) const;
};

}  // namespace griddly