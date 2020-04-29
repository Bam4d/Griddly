#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "GridLocation.hpp"

#define BehaviourFunction std::function<BehaviourResult(std::shared_ptr<Action>)>

namespace griddy {

class Grid;
class Action;

struct BehaviourResult {
  bool abortAction = false;
  int reward = 0;
};

class Object : public std::enable_shared_from_this<Object> {
 public:
  GridLocation getLocation() const;

  // playerId of 0 means the object does not belong to any player in particular, (walls etc)
  void init(uint playerId, GridLocation location, std::shared_ptr<Grid> grid_);

  std::string getObjectName() const;

  uint getObjectId() const;

  std::string getDescription() const;

  uint getPlayerId() const;

  bool canPerformAction(std::string actionName);

  BehaviourResult onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action);

  BehaviourResult onActionDst(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action);

  void addActionSrcBehaviour(std::string action, std::string destinationObjectName, std::string commandName, std::vector<std::string> commandParameters);
  void addActionSrcBehaviour(std::string action, std::string destinationObjectName, std::string commandName, std::vector<std::string> commandParameters, std::unordered_map<std::string, std::vector<std::string>> nestedCommands);

  void addActionDstBehaviour(std::string action, std::string sourceObjectName, std::string commandName, std::vector<std::string> commandParameters);
  void addActionDstBehaviour(std::string action, std::string sourceObjectName, std::string commandName, std::vector<std::string> commandParameters, std::unordered_map<std::string, std::vector<std::string>> nestedCommands);

  Object(std::string objectName, uint id_, std::unordered_map<std::string, std::shared_ptr<uint>> availableParameters);

  ~Object();

 private:
  uint x_;
  uint y_;
  uint playerId_;
  const std::string objectName_;
  const uint id_;

  std::unordered_map<std::string, std::string> actionMap_;


  // action -> destination -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<BehaviourFunction>>> srcBehaviours_;
  
  // action -> source -> [behaviour functions]
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<BehaviourFunction>>> dstBehaviours_;
  
  // The parameters that are available in the object for behaviour commands to interact with
  std::unordered_map<std::string, std::shared_ptr<uint>> availableParameters_;

  std::shared_ptr<Grid> grid_;

  virtual void moveObject(GridLocation newLocation);

  virtual void removeObject();

  std::vector<std::shared_ptr<uint>> findParameters(std::vector<std::string> parameters);

  BehaviourFunction instantiateBehaviour(std::string commandName, std::vector<std::string> commandParameters);
  BehaviourFunction instantiateConditionalBehaviour(std::string commandName, std::vector<std::string> commandParameters, std::unordered_map<std::string, std::vector<std::string>> subCommands);
};

}  // namespace griddy