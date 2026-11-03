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

#include "../../ECS/ArenaPtr.hpp"
#include "../../ECS/Entity.hpp"
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

class ObjectRules : public std::enable_shared_from_this<ObjectRules>, ConditionResolver<BehaviourCondition> {
 public:
  virtual glm::ivec2 getLocation() const;

  virtual void init(glm::ivec2 location);

  virtual void init(glm::ivec2 location, DiscreteOrientation orientation);

  virtual const std::string& getObjectName() const;

  virtual char getMapCharacter() const;

  virtual const std::string& getObjectRenderTileName() const;

  virtual std::string getDescription() const;

  virtual uint32_t getPlayerId() const;

  virtual int32_t getZIdx() const;

  virtual DiscreteOrientation getObjectOrientation() const;

  virtual bool isPlayerAvatar() const;

  virtual bool isRemoved() const;

  // Handle into the Grid's ECS arena (state block). NULL_ENTITY until the
  // object is added to a grid.
  EntityId getEntityId() const { return entityId_; }
  void setEntityId(EntityId entityId) { entityId_ = entityId; }

  // Bind this object's variable storage to its entity slot in the ECS arena:
  // current values are copied into the arena, then every variable pointer
  // (x_, y_, orientation, playerId_, renderTileId_, availableVariables_) is
  // re-aliased to arena memory through a single shared no-op control block,
  // so subsequent reads/writes hit the state block directly with zero
  // additional heap allocations.
  void bindToECS(class ECSWorld* world);
  bool isECSBound() const { return ecsBound_; }

  // Retarget this rules instance to another grid's arena WITHOUT rebuilding
  // anything: offsets stay the same, only the base pointer and grid link
  // change. Used by fast arena clones.
  void rebindGrid(std::shared_ptr<Grid> grid);

  // Non-virtual accessors reading the base members directly. Used by the
  // ECS sync path so it is safe to call on test mocks (which override the
  // virtual getters with gmock methods that throw on uninteresting calls).
  glm::ivec2 locationRef() const { return {*x_, *y_}; }  // arena-backed via handles
  const DiscreteOrientation& orientationRef() const { return orientation_; }
  int32_t playerIdValue() const { return *playerId_; }
  int32_t renderTileIdValue() const { return *renderTileId_; }
  int32_t zIdxValue() const { return zIdx_; }
  const VariableMap& availableVariablesRef() const { return availableVariables_; }
  void setLocationRef(glm::ivec2 location) {
    *x_ = location.x;
    *y_ = location.y;
  }

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

  virtual IntVar getVariableValue(std::string variableName);

  virtual VariableMap getAvailableVariables() const;

  virtual std::unordered_set<std::string> getAvailableActionNames() const;

  // Initial actions for objects
  virtual std::vector<std::shared_ptr<Action>> getInitialActions(std::shared_ptr<Action> originatingAction);
  virtual void setInitialActionDefinitions(std::vector<InitialActionDefinition> actionDefinitions);

  // Conditional functions
  BehaviourResult executeBehaviourFunctionList(std::unordered_map<uint32_t, int32_t>& rewardAccumulator, const std::vector<BehaviourFunction>& behaviourList, const std::shared_ptr<Action>& action) const;

  ObjectRules(const std::string& objectName, char mapCharacter, uint32_t playerId, uint32_t zIdx, const VariableMap& availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator, std::weak_ptr<Grid> grid);
  
  virtual ~ObjectRules();

 private:

  // Have to be shared pointers because they are used as variables
  IntVar x_;
  IntVar y_;


  DiscreteOrientation orientation_;

  IntVar playerId_;
  IntVar renderTileId_;
  const std::string objectName_;
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
  VariableMap availableVariables_;

  std::shared_ptr<Grid> grid() const;
  std::weak_ptr<Grid> grid_;

  std::unordered_set<std::string> availableActionNames_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  virtual bool moveObject(glm::ivec2 newLocation);

  virtual void removeObject();
  bool removed_ = false;

  EntityId entityId_ = NULL_ENTITY;
  bool ecsBound_ = false;

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