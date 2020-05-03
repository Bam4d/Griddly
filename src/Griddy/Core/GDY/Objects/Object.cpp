#include "Object.hpp"

#include <spdlog/spdlog.h>

#include "../../Grid.hpp"
#include "../Actions/Action.hpp"

namespace griddy {

class Action;

GridLocation Object::getLocation() const {
  GridLocation location(x_, y_);
  return location;
};

void Object::init(uint32_t playerId, GridLocation location, std::shared_ptr<Grid> grid) {
  x_ = location.x;
  y_ = location.y;

  grid_ = grid;

  playerId_ = playerId;
}

uint32_t Object::getObjectId() const {
  return id_;
}

std::string Object::getDescription() const {
  return fmt::format("Object: {0} @ [{1}, {2}]", objectName_, x_, y_);
}

BehaviourResult Object::onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  auto actionName = action->getActionName();
  auto objectName = destinationObject == nullptr ? "_empty" : destinationObject->getObjectName();

  auto behavioursForActionIt = srcBehaviours_.find(actionName);
  if (behavioursForActionIt == srcBehaviours_.end()) {
    return {true, 0};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(objectName);
  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    return {true, 0};
  }

  spdlog::debug("Executing behaviours for source {0} -> {1} -> {2}", getObjectName(), actionName, objectName);
  auto &behaviours = behavioursForActionAndDestinationObject->second;

  int rewards = 0;
  for (auto &behaviour : behaviours) {
    auto result = behaviour(action);

    rewards += result.reward;
    if (result.abortAction) {
      return {true, rewards};
    }
  }

  return {false, rewards};
}

BehaviourResult Object::onActionDst(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
  auto actionName = action->getActionName();
  auto objectName = sourceObject == nullptr ? "_empty" : sourceObject->getObjectName();

  auto behavioursForActionIt = dstBehaviours_.find(actionName);
  if (behavioursForActionIt == dstBehaviours_.end()) {
    return {true, 0};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(objectName);
  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    return {true, 0};
  }

  spdlog::debug("Executing behaviours for source {0} -> {1} -> {2}", getObjectName(), actionName, objectName);
  auto &behaviours = behavioursForActionAndDestinationObject->second;

  int rewards = 0;
  for (auto &behaviour : behaviours) {
    auto result = behaviour(action);

    rewards += result.reward;
    if (result.abortAction) {
      return {true, rewards};
    }
  }

  return {false, rewards};
}

std::vector<std::shared_ptr<int32_t>> Object::findParameters(std::vector<std::string> parameters) {
  std::vector<std::shared_ptr<int32_t>> resolvedParams;
  for (auto &param : parameters) {
    if (param == "_dest") {
      resolvedParams.push_back(availableParameters_["_x"]);
      resolvedParams.push_back(availableParameters_["_y"]);
    } else {
      auto parameter = availableParameters_.find(param);
      if (parameter == availableParameters_.end()) {
        throw std::invalid_argument(fmt::format("Undefined parameter {0} for object {1}", param));
      }

      auto resolvedParam = parameter->second;

      resolvedParams.push_back(resolvedParam);
    }
  }

  return resolvedParams;
}

BehaviourFunction Object::instantiateConditionalBehaviour(std::string commandName, std::vector<std::string> commandParameters, std::unordered_map<std::string, std::vector<std::string>> subCommands) {
  if (subCommands.size() == 0) {
    return instantiateBehaviour(commandName, commandParameters);
  }

  if (commandName == "eq") {
    auto parameterPointers = findParameters(commandParameters);

    std::vector<std::unique_ptr<BehaviourFunction>> behaviourFunctions;

    // for (auto subCommand : subCommands) {
    //   auto subCommandName = subCommand.first;
    //   auto subCommandParams = subCommand.second;

    //   behaviourFunctions.push_back(instantiateBehaviour(subCommandName, subCommandParams));
    // }
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto a = *(parameterPointers[0]);
      auto b = *(parameterPointers[1]);

      if (a == b) {
        int rewards = 0;
        // for (auto &function : behaviourFunctions_) {
        //   auto behaviourResult = (*function)(action);
        //   rewards += behaviourResult.reward;
        //   if (behaviourResult.abortAction) {
        //     return BehaviourResult{true, rewards};
        //   }
        // }

        return BehaviourResult{false, rewards};
      }

      return BehaviourResult{false, 0};
    };
  }

  throw std::invalid_argument(fmt::format("Unknown or badly defined command {0}.", commandName));
}

BehaviourFunction Object::instantiateBehaviour(std::string commandName, std::vector<std::string> commandParameters) {
  if (commandName == "reward") {
    auto value = std::stoi(commandParameters[0]);
    return [this, value](std::shared_ptr<Action> action) {
      return BehaviourResult{false, value};
    };
  }

  if (commandName == "override") {
    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto abortAction = (*parameterPointers[0]) != 0;
      auto reward = (*parameterPointers[1]);
      return BehaviourResult{abortAction, reward};
    };
  }

  if (commandName == "incr") {
    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      (*parameterPointers[0]) += 1;
      return BehaviourResult();
    };
  }

  if (commandName == "decr") {
    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      (*parameterPointers[0]) += 1;
      return BehaviourResult();
    };
  }

  if (commandName == "mov") {
    if (commandParameters[0] == "_dest") {
      return [this](std::shared_ptr<Action> action) {
        this->moveObject(action->getDestinationLocation());
        return BehaviourResult();
      };
    }

    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto x = (uint32_t)(*parameterPointers[0]);
      auto y = (uint32_t)(*parameterPointers[1]);

      this->moveObject({x, y});
      return BehaviourResult();
    };
  }

  if (commandName == "cascade") {
    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      auto x = (uint32_t)(*parameterPointers[0]);
      auto y = (uint32_t)(*parameterPointers[1]);

      auto destLocation = GridLocation{x, y};

      auto cascadedAction = std::shared_ptr<Action>(new Action(action->getActionName(), destLocation, action->getDirection()));

      auto rewards = grid_->performActions(0, {cascadedAction});

      int reward = 0;
      for (int r = 0; r < rewards.size(); r++) {
        reward += rewards[r];
      }

      return BehaviourResult{false, reward};
    };
  }

  if (commandName == "remove") {
    auto parameterPointers = findParameters(commandParameters);
    return [this, parameterPointers](std::shared_ptr<Action> action) {
      this->removeObject();
      return BehaviourResult();
    };
  }

  throw std::invalid_argument(fmt::format("Unknown or badly defined command {0}.", commandName));
}

void Object::addActionSrcBehaviour(
    std::string actionName,
    std::string destinationObjectName,
    std::string commandName,
    std::vector<std::string> commandParameters,
    std::unordered_map<std::string, std::vector<std::string>> conditionalCommands) {
  spdlog::debug("Adding behaviour commandName {0} when action {1} is performed on {2}", commandName, actionName, destinationObjectName);

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandParameters, conditionalCommands);
  srcBehaviours_[actionName][destinationObjectName].push_back(behaviourFunction);
}

void Object::addActionDstBehaviour(
    std::string actionName,
    std::string sourceObjectName,
    std::string commandName,
    std::vector<std::string> commandParameters,
    std::unordered_map<std::string, std::vector<std::string>> conditionalCommands) {
  spdlog::debug("Adding behaviour commandName {0} when {1} performs action {2} ", commandName, sourceObjectName, actionName);

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandParameters, conditionalCommands);
  dstBehaviours_[actionName][sourceObjectName].push_back(behaviourFunction);
}

bool Object::canPerformAction(std::string actionName) const {
  auto it = srcBehaviours_.find(actionName);
  return it != srcBehaviours_.end();
}

uint32_t Object::getPlayerId() const {
  return playerId_;
}

void Object::moveObject(GridLocation newLocation) {
  if(grid_->updateLocation(shared_from_this(), {x_, y_}, newLocation)) {
    x_ = newLocation.x;
    y_ = newLocation.y;
  }
}

void Object::removeObject() {
  grid_->removeObject(shared_from_this());
}

std::string Object::getObjectName() const {
  return objectName_;
}

Object::Object(std::string objectName, uint32_t id, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters) : objectName_(objectName), id_(id) {
  availableParameters.insert({"_x", std::make_shared<int32_t>(x_)});
  availableParameters.insert({"_y", std::make_shared<int32_t>(y_)});

  availableParameters_ = availableParameters;
}

Object::~Object() {}

}  // namespace griddy