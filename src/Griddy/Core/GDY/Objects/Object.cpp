#include "Object.hpp"

#include <spdlog/spdlog.h>

#include "../../Grid.hpp"
#include "../Actions/Action.hpp"

namespace griddy {

class Action;

GridLocation Object::getLocation() const {
  GridLocation location(*x_, *y_);
  return location;
};

void Object::init(uint32_t playerId, GridLocation location, std::shared_ptr<Grid> grid) {
  *x_ = location.x;
  *y_ = location.y;

  grid_ = grid;

  playerId_ = playerId;
}

uint32_t Object::getObjectId() const {
  return id_;
}

std::string Object::getDescription() const {
  return fmt::format("{0}@[{1}, {2}]", objectName_, *x_, *y_);
}

BehaviourResult Object::onActionSrc(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {
  auto actionName = action->getActionName();
  auto destinationObjectName = destinationObject == nullptr ? "_empty" : destinationObject->getObjectName();

  auto behavioursForActionIt = srcBehaviours_.find(actionName);
  if (behavioursForActionIt == srcBehaviours_.end()) {
    return {true, 0};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(destinationObjectName);
  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    return {true, 0};
  }

  spdlog::debug("Executing behaviours for source [{0}] -> {1} -> {2}", getObjectName(), actionName, destinationObjectName);
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
  auto sourceObjectName = sourceObject == nullptr ? "_empty" : sourceObject->getObjectName();

  auto behavioursForActionIt = dstBehaviours_.find(actionName);
  if (behavioursForActionIt == dstBehaviours_.end()) {
    return {true, 0};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(sourceObjectName);
  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    return {true, 0};
  }

  spdlog::debug("Executing behaviours for destination {0} -> {1} -> [{2}]", sourceObjectName, actionName, getObjectName());
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
    auto parameter = availableParameters_.find(param);
    std::shared_ptr<int32_t> resolvedParam;

    if (parameter == availableParameters_.end()) {
      spdlog::debug("Parameter string not found, trying to parse literal={0}", param);

      try {
        resolvedParam = std::make_shared<int32_t>(std::stoi(param));
      } catch (const std::exception &e) {
        auto error = fmt::format("Undefined parameter={0}", param);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
    } else {
      resolvedParam = parameter->second;
    }

    resolvedParams.push_back(resolvedParam);
  }

  return resolvedParams;
}

BehaviourFunction Object::instantiateConditionalBehaviour(std::string commandName, std::vector<std::string> commandParameters, std::unordered_map<std::string, std::vector<std::string>> subCommands) {
  if (subCommands.size() == 0) {
    return instantiateBehaviour(commandName, commandParameters);
  }

  std::function<bool(int32_t, int32_t)> condition;
  if (commandName == "eq") {
    condition = [](int32_t a, int32_t b) { return a == b; };
  } else if (commandName == "gt") {
    condition = [](int32_t a, int32_t b) { return a > b; };
  } else if (commandName == "lt") {
    condition = [](int32_t a, int32_t b) { return a < b; };
  } else {
    throw std::invalid_argument(fmt::format("Unknown or badly defined condition command {0}.", commandName));
  }

  auto parameterPointers = findParameters(commandParameters);

  std::vector<BehaviourFunction> conditionalBehaviours;

  for (auto subCommand : subCommands) {
    auto subCommandName = subCommand.first;
    auto subCommandParams = subCommand.second;

    conditionalBehaviours.push_back(instantiateBehaviour(subCommandName, subCommandParams));
  }

  return [this, condition, conditionalBehaviours, parameterPointers](std::shared_ptr<Action> action) {
    auto a = *(parameterPointers[0]);
    auto b = *(parameterPointers[1]);

    if (condition(a, b)) {
      int32_t rewards = 0;
      for (auto &behaviour : conditionalBehaviours) {
        auto result = behaviour(action);

        rewards += result.reward;
        if (result.abortAction) {
          return BehaviourResult{true, rewards};
        }
      }

      return BehaviourResult{false, rewards};
    }

    return BehaviourResult{true, 0};
  };
}

BehaviourFunction Object::instantiateBehaviour(std::string commandName, std::vector<std::string> commandParameters) {
  // Command just used in tests
  if (commandName == "nop") {
    return [this](std::shared_ptr<Action> action) {
      return BehaviourResult{false, 0};
    };
  }

  if (commandName == "reward") {
    auto value = std::stoi(commandParameters[0]);
    return [this, value](std::shared_ptr<Action> action) {
      return BehaviourResult{false, value};
    };
  }

  if (commandName == "override") {
    auto abortAction = commandParameters[0] == "true";
    auto reward = std::stoi(commandParameters[1]);
    return [this, abortAction, reward](std::shared_ptr<Action> action) {
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
      (*parameterPointers[0]) -= 1;
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

    if (commandParameters[0] == "_src") {
      return [this](std::shared_ptr<Action> action) {
        this->moveObject(action->getSourceLocation());
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
    return [this, commandParameters](std::shared_ptr<Action> action) {
      if (commandParameters[0] == "_dest") {
        auto cascadeLocation = action->getDestinationLocation();
        auto cascadedAction = std::shared_ptr<Action>(new Action(action->getActionName(), cascadeLocation, action->getDirection()));

        auto cascadedSrcObject = grid_->getObject(cascadeLocation);
        auto cascadedDstObject = grid_->getObject(cascadedAction->getDestinationLocation());
        return cascadedSrcObject->onActionSrc(cascadedDstObject, cascadedAction);
      }

      spdlog::warn("The only supported parameter for cascade is _dest.");

      return BehaviourResult{true, 0};
    };
  }

  if (commandName == "remove") {
    return [this](std::shared_ptr<Action> action) {
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
  spdlog::debug("Adding behaviour command={0} when action={1} is performed on object={2} by object={3}", commandName, actionName, destinationObjectName, getObjectName());

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandParameters, conditionalCommands);
  srcBehaviours_[actionName][destinationObjectName].push_back(behaviourFunction);
}

void Object::addActionDstBehaviour(
    std::string actionName,
    std::string sourceObjectName,
    std::string commandName,
    std::vector<std::string> commandParameters,
    std::unordered_map<std::string, std::vector<std::string>> conditionalCommands) {
  spdlog::debug("Adding behaviour command={0} when object={1} performs action={2} on object={3}", commandName, sourceObjectName, actionName, getObjectName());

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandParameters, conditionalCommands);
  dstBehaviours_[actionName][sourceObjectName].push_back(behaviourFunction);
}

bool Object::canPerformAction(std::string actionName) const {
  auto it = srcBehaviours_.find(actionName);
  return it != srcBehaviours_.end();
}

std::shared_ptr<int32_t> Object::getParamValue(std::string paramName) {
  auto it = availableParameters_.find(paramName);
  if (it == availableParameters_.end()) {
    return nullptr;
  }

  return it->second;
}

uint32_t Object::getPlayerId() const {
  return playerId_;
}

void Object::moveObject(GridLocation newLocation) {
  if (grid_->updateLocation(shared_from_this(), {(uint32_t)*x_, (uint32_t)*y_}, newLocation)) {
    *x_ = newLocation.x;
    *y_ = newLocation.y;
  }
}

void Object::removeObject() {
  grid_->removeObject(shared_from_this());
}

uint32_t Object::getZIdx() const {
  return zIdx_;
}

std::string Object::getObjectName() const {
  return objectName_;
}

bool Object::isPlayerAvatar() const {
  return isPlayerAvatar_;
}

void Object::markAsPlayerAvatar() {
  isPlayerAvatar_ = true;
}

Object::Object(std::string objectName, uint32_t id, uint32_t zIdx, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters) : objectName_(objectName), id_(id), zIdx_(zIdx) {
  availableParameters.insert({"_x", x_});
  availableParameters.insert({"_y", y_});

  availableParameters_ = availableParameters;
}

Object::~Object() {}

}  // namespace griddy