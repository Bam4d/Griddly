#pragma once

#include <glm/glm.hpp>
#include <string>

#include "../../Grid.hpp"
#include "../Objects/Object.hpp"
#include "Direction.hpp"

namespace griddly {

class Grid;

// How the action is initialized so destination and sources can be retrieved
enum class ActionMode {
  SRC_LOC_DST_LOC,
  SRC_OBJ_DST_LOC,
  SRC_OBJ_DST_OBJ,
  SRC_OBJ_DST_VEC
};

class Action {
 public:
  Action(std::shared_ptr<Grid> grid, std::string actionName, uint32_t delay = 0);

  // An action that is not tied to any specific units in the grid, these actions can be performed by the environment, or can be RTS input
  virtual void init(glm::ivec2 sourceLocation, glm::ivec2 destinationLocation);

  // An action that has a specified source object and a location, this can be used for most actions
  virtual void init(std::shared_ptr<Object> sourceObject, glm::ivec2 destinationLocation);

  // An action tied to a specific source and destination object
  virtual void init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject);

  // action used to specify a source object and a direction for an action. Can be used for actions that are relative to avatars.
  virtual void init(std::shared_ptr<Object> sourceObject, glm::ivec2 vector, bool relativeToSource);

  // action used to specify a source object and destination object and a direction for an action. 
  // Can be used for actions where the destination of the action is not the same as the vector such as rotating the current object or pulling another object
  // TODO: not sure if this is the best thing wat of doing this?
  virtual void init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject, glm::ivec2 vector, bool relativeToSource);

  // resolve the source object in the current grid
  virtual std::shared_ptr<Object> getSourceObject() const;

  // resolve the destination object in the current grid
  virtual std::shared_ptr<Object> getDestinationObject() const;

  virtual glm::ivec2 getSourceLocation() const;
  
  virtual glm::ivec2 getDestinationLocation() const;

  virtual glm::ivec2 getVector() const;

  virtual std::string getActionName() const;

  virtual std::string getDescription() const;

  // Delay an action
  virtual uint32_t getDelay() const;

  ~Action();

 protected:
  std::shared_ptr<Object> sourceObject_ = nullptr;
  std::shared_ptr<Object> destinationObject_ = nullptr;
  glm::ivec2 destinationLocation_ = {0, 0};
  glm::ivec2 sourceLocation_ = {0, 0};
  glm::ivec2 vector_ = {0, 0};

  DiscreteOrientation orientation_;

  const std::string actionName_;
  const uint32_t delay_;
  const std::shared_ptr<Grid> grid_;

 private:
  ActionMode actionMode_;
};
}  // namespace griddly