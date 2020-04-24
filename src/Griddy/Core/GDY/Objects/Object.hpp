#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include <functional>

namespace griddy {

namespace gdy {

class Action;
class Grid;

class Object : public std::enable_shared_from_this<Object> {
 public:
  GridLocation getLocation() const;

  // playerId of 0 means the object does not belong to any player in particular, (walls etc)
  void init(uint playerId, GridLocation location, std::shared_ptr<Grid> grid_);

  uint getObjectType() const;
  
  std::string getDescription() const;

  uint getPlayerId() const;

  // An action has been performed on this object by another object, such as a movement, harvest, attack etc
  // bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) = 0;

  // This object has performed an action
  // int onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action);

  // Can this object perform any action
  // bool canPerformAction(std::shared_ptr<Action> action);

  int onActionSrc(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action);

  bool onActionDst(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action);

  bool addActionSrcBehaviour(std::string action, std::function<bool> behaviour);

  bool addActionDstBehaviour(std::string action, std::function<bool> behaviour);

  Object(std::string objectName, std::unordered_map<std::string, std::shared_ptr<uint>> parameters);

  ~Object();

 private:
  uint x_;
  uint y_;
  uint playerId_;

  const std::string objectName_;
  std::unordered_map<std::string, std::shared_ptr<uint>> parameters_;

  std::unordered_map<std::string, std::function<bool>> dstBehavours_;

  std::shared_ptr<Grid> grid_;

  virtual void moveObject(GridLocation newLocation);

  virtual void removeObject();
};

}  // namespace gdy

}  // namespace griddy