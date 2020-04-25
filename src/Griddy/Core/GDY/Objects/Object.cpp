#include "Object.hpp"

#include "../../Grid.hpp"
#include "../Actions/Action.hpp"

namespace griddy {

class Action;

GridLocation Object::getLocation() const {
  GridLocation location(x_, y_);
  return location;
};

void Object::init(uint playerId, GridLocation location, std::shared_ptr<Grid> grid) {
  x_ = location.x;
  y_ = location.y;

  grid_ = grid;

  playerId_ = playerId;
}

BehaviourResult Object::onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  return {};
}

BehaviourResult Object::onActionDst(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
  return {};
}

std::vector<std::shared_ptr<uint>> Object::findParameters(std::vector<std::string> parameters) {
}

std::function<BehaviourResult(std::shared_ptr<Action>)> Object::instantiateBehaviour(std::string action, std::string commandName, std::vector<std::string> commandParameters) {
  auto parameterPointers = findParameters(commandParameters);

  if (commandName == "reward") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      return BehaviourResult{false, 1};
    };
  }

  if (commandName == "override") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto abortAction = (*parameterPointers[0]) != 0;
      auto reward = (*parameterPointers[1]);
      return BehaviourResult{abortAction, reward};
    };
  }

  if (commandName == "incr") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      (*parameterPointers[0]) += 1;
      return BehaviourResult();
    };
  }

  if (commandName == "decr") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      (*parameterPointers[0]) += 1;
      return BehaviourResult();
    };
  }

  if (commandName == "mov") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto x = (*parameterPointers[0]);
      auto y = (*parameterPointers[1]);

      this->moveObject({x, y});
      return BehaviourResult();
    };
  }

  if (commandName == "cascade") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto x = (*parameterPointers[0]);
      auto y = (*parameterPointers[1]);

      auto destLocation = GridLocation{x, y};

      auto cascadedAction = std::shared_ptr<Action>(new Action(action->getActionName(), destLocation, action->getDirection()));

      auto rewards = grid_->performActions(0, {cascadedAction});

      int reward = 0;
      for (int r = 0; r<rewards.size(); r++) {
        reward += rewards[r];
      }

      return BehaviourResult{false, reward};
    };
  }

  if (commandName == "eq") {
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto a = *(parameterPointers[0]);
      auto b = *(parameterPointers[1]);

      auto subCommand = 
      return 
    };
  }
}

void Object::addActionSrcBehavour(std::string action, std::string commandName, std::vector<std::string> commandParameters) {
  srcBehavours_.insert({action, parameters});
}

void Object::addActionDstBehavour(std::string action, std::vector<std::shared_ptr<uint>> parameters) {
  dstBehavours_.insert({action, parameters});
}

uint Object::getPlayerId() const {
  return playerId_;
}

void Object::moveObject(GridLocation newLocation) {
  grid_->updateLocation(shared_from_this(), {x_, y_}, newLocation);
  x_ = newLocation.x;
  y_ = newLocation.y;
}

void Object::removeObject() {
  grid_->removeObject(shared_from_this());
}

Object::Object(std::string objectName, std::unordered_map<std::string, std::shared_ptr<uint>> availableParameters) : objectName_(objectName) {
  availableParameters.insert({"_x", std::shared_ptr<uint>(x_)});
  availableParameters.insert({"_y", std::shared_ptr<uint>(y_)});

  availableParameters_ = availableParameters;
}

Object::~Object() {}

}  // namespace griddy