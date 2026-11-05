#pragma once

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CollisionDetectorFactory.hpp"
#include "DelayedActionQueueItem.hpp"
#include "ECS/ECSWorld.hpp"
#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/ObjectRules.hpp"
#include "LevelGenerators/LevelGenerator.hpp"
#include "Util/RandomGenerator.hpp"
#include "Util/util.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define TileObjects std::map<uint32_t, std::shared_ptr<ObjectRules>>

namespace griddly {

enum class TriggerType {
  NONE,
  RANGE_BOX_BOUNDARY,
  RANGE_BOX_AREA,
};

struct ActionTriggerDefinition {
  std::unordered_set<std::string> sourceObjectNames;
  std::unordered_set<std::string> destinationObjectNames;
  TriggerType triggerType = TriggerType::RANGE_BOX_AREA;
  uint32_t range = 1;
  bool relative = false;
  glm::ivec2 offset{};
};

// Structure to hold information about the events that have happened at each time step
struct GridEvent {
  uint32_t playerId;
  std::string actionName;
  uint32_t tick = 0;
  std::unordered_map<uint32_t, int32_t> rewards;
  uint32_t delay = 0;

  std::string sourceObjectName;
  std::string destObjectName;

  uint32_t sourceObjectPlayerId = 0;
  uint32_t destinationObjectPlayerId = 0;

  glm::ivec2 sourceLocation;
  glm::ivec2 destLocation;
};

struct GlobalVariableDefinition {
  int32_t initialValue = 0;
  bool perPlayer = false;
};

class Grid : public std::enable_shared_from_this<Grid> {
 public:
  Grid();
  explicit Grid(std::shared_ptr<CollisionDetectorFactory> collisionDetectorFactory);
  virtual ~Grid();

  virtual void setPlayerCount(uint32_t playerCount);
  virtual uint32_t getPlayerCount() const;
  virtual void resetMap(uint32_t width, uint32_t height);
  virtual void resetGlobalVariables(const std::map<std::string, GlobalVariableDefinition>& globalVariableDefinitions);
  virtual void setGlobalVariables(const std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>>& globalVariableDefinitions);

  virtual std::unordered_map<uint32_t, int32_t> performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions);

  // Internal array-based variants: no per-action heap map allocations.
  // The public map-returning versions just wrap these with toRewardMap.
  RewardArray performActionsArr(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions);
  RewardArray executeActionArr(uint32_t playerId, const std::shared_ptr<Action>& action);
  RewardArray executeAndRecordArr(uint32_t playerId, const std::shared_ptr<Action>& action);

  // Pooled action path: zero heap allocations per action. acquireAction()
  // reuses a preallocated Action slot; performActionsPooled() wraps them in
  // zero-alloc aliasing shared_ptrs, runs the normal path, and releases.
  Action* acquireAction(const std::string& actionName, uint32_t playerId, uint32_t delay = 0);
  void releaseActions(const std::vector<Action*>& actions);
  std::unordered_map<uint32_t, int32_t> performActionsPooled(uint32_t playerId, const std::vector<Action*>& actions);
  virtual std::unordered_map<uint32_t, int32_t> executeAction(uint32_t playerId, std::shared_ptr<Action> action);
  virtual void delayAction(uint32_t playerId, std::shared_ptr<Action> action);

  virtual std::unordered_map<uint32_t, int32_t> update();
  virtual std::unordered_map<uint32_t, int32_t> processDelayedActions();

  virtual std::unordered_map<uint32_t, int32_t> processCollisions();
  virtual void addActionTrigger(std::string actionName, ActionTriggerDefinition actionTriggerDefinition);
  virtual void setBehaviourProbabilities(const std::unordered_map<std::string, std::vector<float>>& behaviourProbabilities);

  virtual const DelayedActionQueue& getDelayedActions();

  virtual bool updateLocation(std::shared_ptr<ObjectRules> object, glm::ivec2 previousLocation, glm::ivec2 newLocation);

  // Mark a particular location to be repainted
  virtual bool invalidateLocation(glm::ivec2 location);

  virtual const std::unordered_set<glm::ivec2>& getUpdatedLocations(uint32_t player) const;
  virtual void purgeUpdatedLocations(uint32_t player);

  virtual uint32_t getWidth() const;
  virtual uint32_t getHeight() const;

  virtual std::shared_ptr<int32_t> getTickCount() const;
  virtual void setTickCount(int32_t tickCount);

  virtual void initObject(std::string objectName, std::vector<std::string> objectVariableNames);

  virtual void addObject(glm::ivec2 location, std::shared_ptr<ObjectRules> object, bool applyInitialActions = true, std::shared_ptr<Action> originatingAction = nullptr, DiscreteOrientation orientation = DiscreteOrientation());

  virtual bool removeObject(std::shared_ptr<ObjectRules> object);

  virtual const std::vector<std::shared_ptr<ObjectRules>>& getObjects();

  virtual void addPlayerDefaultEmptyObject(std::shared_ptr<ObjectRules> emptyObject);
  
  virtual void addPlayerDefaultBoundaryObject(std::shared_ptr<ObjectRules> boundaryObject);

  virtual std::shared_ptr<ObjectRules> getPlayerDefaultEmptyObject(uint32_t playerId) const;

  virtual std::shared_ptr<ObjectRules> getPlayerDefaultBoundaryObject(uint32_t playerId) const;

  /**
   * Gets all the objects at a certain location
   */
  virtual const TileObjects& getObjectsAt(glm::ivec2 location) const;

  /**
   * Gets the object with the highest Z index at a certain tile location
   */
  virtual std::shared_ptr<ObjectRules> getObject(glm::ivec2 location) const;

  /**
   * Get a list of the objects and their Ids in this grid
   */
  virtual const std::unordered_map<std::string, uint32_t>& getObjectIds() const;

  /**
   * Get a list of the object variables and their Ids in this grid
   */
  virtual const std::unordered_map<std::string, uint32_t>& getObjectVariableIds() const;

  /**
   * Gets an ordered list of objectVariableNames
   */
  virtual const std::vector<std::string> getAllObjectVariableNames() const;

  /**
   * Get a mapping of objects to their defined variables
   */
  virtual const std::unordered_map<std::string, std::vector<std::string>> getObjectVariableMap() const;

  /**
   * Gets an ordered list of objectNames
   */
  virtual const std::vector<std::string> getObjectNames() const;

  /**
   * Get a mapping of the avatar objects for players in the environment
   */
  virtual std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> getPlayerAvatarObjects() const;

  virtual std::unordered_map<uint32_t, std::shared_ptr<int32_t>> getObjectCounter(std::string objectName);

  virtual const std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>& getGlobalVariables() const;

  virtual void enableHistory(bool enable);
  virtual const std::vector<GridEvent>& getHistory() const;
  virtual void purgeHistory();

  // These are public so they can be tested
  virtual const std::unordered_map<std::string, std::shared_ptr<CollisionDetector>>& getCollisionDetectors() const;
  virtual const std::unordered_map<std::string, ActionTriggerDefinition>& getActionTriggerDefinitions() const;
  virtual const std::unordered_map<std::string, std::unordered_set<std::string>>& getSourceObjectCollisionActionNames() const;
  virtual const std::unordered_map<std::string, std::unordered_set<std::string>>& getObjectCollisionActionNames() const;

  virtual void addCollisionDetector(std::unordered_set<std::string> objectNames, std::string actionName, std::shared_ptr<CollisionDetector> collisionDetector);

  virtual void reset();

  virtual void seedRandomGenerator(uint32_t seed);

  virtual std::shared_ptr<RandomGenerator> getRandomGenerator() const;

  // ----- ECS state block -----
  // The ECSWorld arena mirrors all mutable game state (entities, locations,
  // variables, tick, delayed actions) in one fixed-capacity block of bytes.
  virtual std::shared_ptr<ECSWorld> getECSWorld() const;

  // Serialize/restore the entire ECS state block as a chunk of bytes.
  virtual size_t saveStateTo(void* dst);
  virtual bool loadStateFrom(const void* src, size_t size);

  // Capture the current state as the initial state (call after level load);
  // resetToInitialState() then restores it with a single memcpy.
  virtual void captureInitialState();
  virtual void resetToInitialState();

  // Fast clone: copy the ECS state block (one memcpy) and share the
  // ObjectRules graph (no cloneInstance/addObject/bind per object).
  // Occupancy, counters and avatars are rebuilt from the arena.
  virtual void cloneStateFrom(const Grid& src);

  void rebuildOccupancyIfDirty() const;

  // Copy live object state into / out of the ECS arena.
  void syncObjectsToECS();
  void syncECSToObjects();
  void syncObjectToECS(const std::shared_ptr<ObjectRules>& object);

 private:
  GridEvent buildGridEvent(const std::shared_ptr<Action>& action, uint32_t playerId, uint32_t tick) const;
  void recordGridEvent(GridEvent event, std::unordered_map<uint32_t, int32_t> rewards);

  const std::vector<std::shared_ptr<CollisionDetector>> getCollisionDetectorsForObject(std::shared_ptr<ObjectRules> object) const;

  std::unordered_map<uint32_t, int32_t> executeAndRecord(uint32_t playerId, const std::shared_ptr<Action>& action);

  std::vector<uint32_t> filterBehaviourProbabilities(std::vector<uint32_t> actionBehaviourIdxs, std::vector<float> actionProbabilities);

  uint32_t height_{};
  uint32_t width_{};

  const std::shared_ptr<int32_t> gameTicks_;

  // For every game tick record a list of locations that should be updated.
  // This is so we can highly optimize observers to only re-render changed grid locations
  std::vector<std::unordered_set<glm::ivec2>> updatedLocations_;

  // Locations invalidated during the current tick (moves/spawns/removals).
  // Used to skip collision checks for sources whose search area is untouched.
  std::unordered_set<glm::ivec2> tickDirtyLocations_;
  // Sources that had at least one collision last tick: they must be re-checked
  // every tick so persistent collisions keep firing (same semantics as a full scan).
  std::unordered_set<std::shared_ptr<ObjectRules>> activeCollisionSources_;
  bool collisionFullScanPending_ = true;
  bool nearAnyDirty(glm::ivec2 location, uint32_t range) const;

  std::unordered_map<std::string, uint32_t> objectIds_;
  std::unordered_map<std::string, uint32_t> objectVariableIds_;
  std::unordered_map<std::string, uint32_t> globalVariableIds_;  // name -> arena slot
  std::unordered_map<std::string, std::vector<std::string>> objectVariableMap_;
  // Compact object list, reserved to maxEntities at resetMap: zero
  // per-object heap allocations on add/clone.
  std::vector<std::shared_ptr<ObjectRules>> objects_;
  // Occupancy map is rebuilt lazily from the arena (occupancyDirty_).
  mutable std::unordered_map<glm::ivec2, TileObjects> occupiedLocations_;
  mutable bool occupancyDirty_ = true;
  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> objectCounters_;
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> playerAvatars_;
  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables_;

  // return reference to this if there are no object in getObjectAt
  const std::map<uint32_t, std::shared_ptr<ObjectRules>> EMPTY_OBJECTS = {};
  const std::unordered_set<glm::ivec2> EMPTY_LOCATIONS = {};

  // A priority queue of actions that are delayed in time (time is measured in game ticks)
  DelayedActionQueue delayedActions_;
  std::unordered_map<std::string, std::vector<float>> behaviourProbabilities_;

  // There is at least 1 player
  uint32_t playerCount_ = 1;

  bool recordEvents_ = false;
  std::vector<GridEvent> eventHistory_;

  // If there are collisions that need to be processed in this game environment

  // All objects that can collide
  std::unordered_map<std::string, std::unordered_set<std::string>> collisionObjectActionNames_;

  // Only the source objects that can collide
  std::unordered_map<std::string, std::unordered_set<std::string>> collisionSourceObjectActionNames_;

  // keep a list of the objects that are named as collision sources, this makes collision processing significantly faster with large maps with many non-colliding objects
  std::unordered_set<std::shared_ptr<ObjectRules>> collisionSourceObjects_;

  // Collision detectors are grouped by action name (i.e each trigger)
  std::shared_ptr<CollisionDetectorFactory> collisionDetectorFactory_;
  std::unordered_map<std::string, std::shared_ptr<CollisionDetector>> collisionDetectors_;
  std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions_;

  // An object that is used if the source of destination location of an action is '_empty'
  // Allows a subset of actions like "spawn" to be performed in empty space.
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> defaultEmptyObject_;

  // An object that is used if the source of destination location of an action is '_boundary'
  // This is used for special actions where objects go out of bounds for the level
  std::unordered_map<uint32_t, std::shared_ptr<ObjectRules>> defaultBoundaryObject_;

  std::shared_ptr<RandomGenerator> randomGenerator_ = std::make_shared<RandomGenerator>(RandomGenerator());

  // ECS arena holding all mutable game state as one block of bytes.
  // Created in resetMap(), sized from the level dimensions.
  std::shared_ptr<ECSWorld> ecsWorld_;

  // Action object pool: allocated once, recycled via free list (no per-tick
  // heap allocations). noopAnchor_ lets us wrap pooled Action* in an
  // aliasing shared_ptr without allocating a control block.
  std::vector<Action*> actionPool_;
  std::vector<Action*> freeActions_;
  std::shared_ptr<Action> noopAnchor_ = std::shared_ptr<Action>(nullptr, [](Action*) {});
 public:
  const std::shared_ptr<Action>& actionAnchor() const { return noopAnchor_; }
 private:
  std::vector<std::shared_ptr<Action>> actionScratch_;

  // action-name hash -> name, so delayed actions mirrored into the arena
  // (which stores only a uint32 hash, keeping the block POD) can be
  // rebuilt into live Action objects on load/reset/clone.
  std::unordered_map<uint32_t, std::string> actionHashToName_;
  static uint32_t actionNameHash(const std::string& name) {
    uint32_t h = 2166136261u;
    for (char c : name) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    return h;
  }
  void mirrorDelayedActionsToECS();
  void rebuildDelayedActionsFromECS();
};

}  // namespace griddly