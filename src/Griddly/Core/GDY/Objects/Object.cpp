#include "Object.hpp"

#include <spdlog/spdlog.h>

#include "../../Grid.hpp"
#include "../Actions/Action.hpp"
#include "ObjectGenerator.hpp"

namespace griddly {

glm::ivec2 Object::getLocation() const {
  glm::ivec2 location(*x_, *y_);
  return location;
};

void Object::init(uint32_t playerId, glm::ivec2 location, std::shared_ptr<Grid> grid) {
  init(playerId, location, DiscreteOrientation(Direction::NONE), grid);
}

void Object::init(uint32_t playerId, glm::ivec2 location, DiscreteOrientation orientation, std::shared_ptr<Grid> grid) {
  *x_ = location.x;
  *y_ = location.y;

  orientation_ = orientation;

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

std::unordered_map<std::string, std::shared_ptr<int32_t>> Object::resolveVariables(BehaviourCommandArguments commandArguments) {
  std::unordered_map<std::string, std::shared_ptr<int32_t>> resolvedVariables;
  for (auto &commandArgument : commandArguments) {
    auto commandArgumentName = commandArgument.first;
    auto commandArgumentValue = commandArgument.second.as<std::string>();
    auto variable = availableVariables_.find(commandArgumentValue);
    std::shared_ptr<int32_t> resolvedVariable;

    if (variable == availableVariables_.end()) {
      spdlog::debug("Variable string not found, trying to parse literal={0}", commandArgumentValue);

      try {
        resolvedVariable = std::make_shared<int32_t>(std::stoi(commandArgumentValue));
      } catch (const std::exception &e) {
        auto error = fmt::format("Undefined variable={0}", commandArgumentValue);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
    } else {
      resolvedVariable = variable->second;
    }

    resolvedVariables[commandArgumentName] = resolvedVariable;
  }

  return resolvedVariables;
}

PreconditionFunction Object::instantiatePrecondition(std::string commandName, BehaviourCommandArguments commandArguments) {
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

  auto variablePointers = resolveVariables(commandArguments);

  auto a = variablePointers["0"];
  auto b = variablePointers["1"];

  return [this, condition, a, b]() {
    return condition(*a, *b);
  };
}

BehaviourFunction Object::instantiateConditionalBehaviour(std::string commandName, BehaviourCommandArguments commandArguments, std::unordered_map<std::string, BehaviourCommandArguments> subCommands) {
  if (subCommands.size() == 0) {
    return instantiateBehaviour(commandName, commandArguments);
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

  auto variablePointers = resolveVariables(commandArguments);

  std::vector<BehaviourFunction> conditionalBehaviours;

  for (auto subCommand : subCommands) {
    auto subCommandName = subCommand.first;
    auto subCommandVariables = subCommand.second;

    conditionalBehaviours.push_back(instantiateBehaviour(subCommandName, subCommandVariables));
  }

  auto a = variablePointers["0"];
  auto b = variablePointers["1"];

  return [this, condition, conditionalBehaviours, a, b](std::shared_ptr<Action> action) {
    if (condition(*a, *b)) {
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

BehaviourFunction Object::instantiateBehaviour(std::string commandName, BehaviourCommandArguments commandArguments) {
  // Command just used in tests
  if (commandName == "nop") {
    return [this](std::shared_ptr<Action> action) {
      return BehaviourResult{false, 0};
    };
  }

  if (commandName == "reward") {
    auto value = commandArguments["0"].as<int32_t>(0);
    return [this, value](std::shared_ptr<Action> action) {
      return BehaviourResult{false, value};
    };
  }

  if (commandName == "override") {
    auto abortAction = commandArguments["0"].as<bool>(false);
    auto reward = commandArguments["1"].as<int32_t>(0);
    return [this, abortAction, reward](std::shared_ptr<Action> action) {
      return BehaviourResult{abortAction, reward};
    };
  }

  if (commandName == "incr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) {
      (*a) += 1;
      return BehaviourResult();
    };
  }

  if (commandName == "change_to") {
    auto objectName = commandArguments["0"].as<std::string>();
    return [this, objectName](std::shared_ptr<Action> action) {
      spdlog::debug("Changing object={0} to {1}", getObjectName(), objectName);
      auto newObject = objectGenerator_->newInstance(objectName, grid_->getGlobalVariables());
      auto playerId = getPlayerId();
      auto location = getLocation();
      removeObject();
      grid_->initObject(playerId, location, newObject);
      return BehaviourResult();
    };
  }

  if (commandName == "decr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) {
      (*a) -= 1;
      return BehaviourResult();
    };
  }

  if (commandName == "rot") {
    if (commandArguments["0"].as<std::string>() == "_dir") {
      return [this](std::shared_ptr<Action> action) {
        orientation_ = DiscreteOrientation(action->getOrientationVector());

        // redraw the current location
        grid_->invalidateLocation(getLocation());
        return BehaviourResult();
      };
    }
  }

  if (commandName == "mov") {
    if (commandArguments["0"].as<std::string>() == "_dest") {
      return [this](std::shared_ptr<Action> action) {
        auto objectMoved = moveObject(action->getDestinationLocation());
        return BehaviourResult{!objectMoved};
      };
    }

    if (commandArguments["0"].as<std::string>() == "_src") {
      return [this](std::shared_ptr<Action> action) {
        auto objectMoved = moveObject(action->getSourceLocation());
        return BehaviourResult{!objectMoved};
      };
    }

    auto variablePointers = resolveVariables(commandArguments);

    if (variablePointers.size() != 2) {
      spdlog::error("Bad mov command detected! There should be two arguments but {0} were provided. This command will be ignored.", variablePointers.size());
      return [this](std::shared_ptr<Action> action) {
        return BehaviourResult{false, 0};
      };
    }

    auto x = variablePointers["0"];
    auto y = variablePointers["1"];

    return [this, x, y](std::shared_ptr<Action> action) {
      auto objectMoved = moveObject({*x, *y});
      return BehaviourResult{!objectMoved};
    };
  }

  if (commandName == "cascade") {
    auto a = commandArguments["0"].as<std::string>();
    return [this, a](std::shared_ptr<Action> action) {
      if (a == "_dest") {
        std::shared_ptr<Action> cascadedAction = std::shared_ptr<Action>(new Action(grid_, action->getActionName(), action->getDelay()));

        cascadedAction->init(action->getDestinationObject(), action->getVectorToDest(), action->getOrientationVector(), false);

        auto rewards = grid_->performActions(0, {cascadedAction});

        int32_t totalRewards = 0;
        for (auto r : rewards) {
          totalRewards += r;
        }

        return BehaviourResult{false, totalRewards};
      }

      spdlog::warn("The only supported variable for cascade is _dest.");

      return BehaviourResult{true, 0};
    };
  }

  if (commandName == "exec") {
    auto actionName = commandArguments["Action"].as<std::string>();
    auto delay = commandArguments["Delay"].as<uint32_t>(0);
    auto relative = commandArguments["Relative"].as<bool>(false);
    auto randomize = commandArguments["Relative"].as<bool>(false);

    glm::ivec2 vectorToDest;
    glm::ivec2 orientationVector;

    auto vectorToDestIt = commandArguments.find("VectorToDest");
    auto orientationVectorIt = commandArguments.find("OrientationVector");

    // TODO: can change these into flags?
    bool inheritVector = vectorToDestIt == commandArguments.end();
    bool inheritOrientation = orientationVectorIt == commandArguments.end();

    bool randomVector = false;
    bool randomOrientation = false;

    if (!inheritVector) {
      auto vectorNode = vectorToDestIt->second;
      vectorToDest[0] = vectorNode[0].as<int32_t>(0);
      vectorToDest[1] = vectorNode[1].as<int32_t>(0);
    }

    if (!inheritOrientation) {
      auto orientationNode = vectorToDestIt->second;
      orientationVector[0] = orientationNode[0].as<int32_t>(0);
      orientationVector[1] = orientationNode[1].as<int32_t>(0);
    }

    // Resolve source object
    return [this, actionName, delay, randomize, inheritVector, vectorToDest, inheritOrientation, orientationVector, relative](std::shared_ptr<Action> action) {
      std::shared_ptr<Action> newAction = std::shared_ptr<Action>(new Action(grid_, actionName, delay));

      glm::ivec2 resolvedVectorToDest = vectorToDest;
      glm::ivec2 resolvedOrientationVector = orientationVector;

      if (randomize) {
        auto randomInputMapping = getRandomInputMapping(actionName);
        resolvedVectorToDest = randomInputMapping.vectorToDest;
        resolvedOrientationVector = randomInputMapping.orientationVector;
      } else {

        if (inheritVector) {
          resolvedVectorToDest = action->getVectorToDest();
        }

        if (inheritOrientation) {
          resolvedOrientationVector = action->getOrientationVector();
        }
      }

      newAction->init(shared_from_this(), resolvedVectorToDest, resolvedOrientationVector, relative);
      auto rewards = grid_->performActions(0, {newAction});

      int32_t totalRewards = 0;
      for (auto r : rewards) {
        totalRewards += r;
      }

      return BehaviourResult{false, totalRewards};
    };
  }

  if (commandName == "remove") {
    return [this](std::shared_ptr<Action> action) {
      removeObject();
      return BehaviourResult();
    };
  }

  throw std::invalid_argument(fmt::format("Unknown or badly defined command {0}.", commandName));
}  // namespace griddly

void Object::addPrecondition(std::string actionName, std::string destinationObjectName, std::string commandName, BehaviourCommandArguments commandArguments) {
  spdlog::debug("Adding action precondition command={0} when action={1} is performed on object={2} by object={3}", commandName, actionName, destinationObjectName, getObjectName());
  auto preconditionFunction = instantiatePrecondition(commandName, commandArguments);
  actionPreconditions_[actionName][destinationObjectName].push_back(preconditionFunction);
}

void Object::addActionSrcBehaviour(
    std::string actionName,
    std::string destinationObjectName,
    std::string commandName,
    BehaviourCommandArguments commandArguments,
    std::unordered_map<std::string, BehaviourCommandArguments> conditionalCommands) {
  spdlog::debug("Adding behaviour command={0} when action={1} is performed on object={2} by object={3}", commandName, actionName, destinationObjectName, getObjectName());

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandArguments, conditionalCommands);
  srcBehaviours_[actionName][destinationObjectName].push_back(behaviourFunction);
}

void Object::addActionDstBehaviour(
    std::string actionName,
    std::string sourceObjectName,
    std::string commandName,
    BehaviourCommandArguments commandArguments,
    std::unordered_map<std::string, BehaviourCommandArguments> conditionalCommands) {
  spdlog::debug("Adding behaviour command={0} when object={1} performs action={2} on object={3}", commandName, sourceObjectName, actionName, getObjectName());

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandArguments, conditionalCommands);
  dstBehaviours_[actionName][sourceObjectName].push_back(behaviourFunction);
}

bool Object::checkPreconditions(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) const {
  auto actionName = action->getActionName();
  auto destinationObjectName = destinationObject == nullptr ? "_empty" : destinationObject->getObjectName();

  spdlog::debug("Checking preconditions for action {0}", actionName);

  // There are no source behaviours for this action, so this action cannot happen
  auto it = srcBehaviours_.find(actionName);
  if (it == srcBehaviours_.end()) {
    return false;
  }

  // Check for preconditions
  auto preconditionsForActionIt = actionPreconditions_.find(actionName);

  // If there are no preconditions then we just let the action happen
  if (preconditionsForActionIt == actionPreconditions_.end()) {
    return true;
  }

  auto &preconditionsForAction = preconditionsForActionIt->second;
  spdlog::debug("{0} preconditions found.", preconditionsForAction.size());

  auto preconditionsForActionAndDestinationObjectIt = preconditionsForAction.find(destinationObjectName);
  if (preconditionsForActionAndDestinationObjectIt == preconditionsForAction.end()) {
    return true;
  }

  spdlog::debug("Checking preconditions for action source [{0}] -> {1} -> {2}", getObjectName(), actionName, destinationObjectName);
  auto &preconditions = preconditionsForActionAndDestinationObjectIt->second;

  for (auto precondition : preconditions) {
    if (!precondition()) {
      return false;
    }
  }

  return true;
}

std::shared_ptr<int32_t> Object::getVariableValue(std::string variableName) {
  auto it = availableVariables_.find(variableName);
  if (it == availableVariables_.end()) {
    return nullptr;
  }

  return it->second;
}

InputMapping Object::getRandomInputMapping(std::string actionName) {
  auto actionMappings = objectGenerator_->getActionMappings();
  auto actionInputMapping = actionMappings[actionName];
  auto inputMapping = actionInputMapping.inputMappings;

  auto it = inputMapping.begin();
  std::advance(it, rand() % inputMapping.size());

  return it->second;
}

void Object::setInitialActionDefinitions(std::vector<InitialActionDefinition> initialActionDefinitions) {
  initialActionDefinitions_ = initialActionDefinitions;
}

std::vector<std::shared_ptr<Action>> Object::getInitialActions() {
  std::vector<std::shared_ptr<Action>> initialActions;
  for (auto actionDefinition : initialActionDefinitions_) {
    auto actionName = actionDefinition.actionName;
    auto actionMappings = objectGenerator_->getActionMappings();
    auto actionInputsDefinition = actionMappings[actionName];
    auto inputMappings = actionInputsDefinition.inputMappings;

    InputMapping inputMapping;
    if (actionDefinition.randomize) {
      inputMapping = getRandomInputMapping(actionName);
    } else {
      inputMapping = inputMappings[actionDefinition.actionId];
    }

    auto action = std::shared_ptr<Action>(new Action(grid_, actionDefinition.actionName, actionDefinition.delay));
    action->init(shared_from_this(), inputMapping.vectorToDest, inputMapping.orientationVector, actionInputsDefinition.relative);

    initialActions.push_back(action);
  }

  return initialActions;
}

uint32_t Object::getPlayerId() const {
  return playerId_;
}

bool Object::moveObject(glm::ivec2 newLocation) {
  if (grid_->updateLocation(shared_from_this(), {*x_, *y_}, newLocation)) {
    *x_ = newLocation.x;
    *y_ = newLocation.y;
    return true;
  }

  return false;
}

void Object::removeObject() {
  grid_->removeObject(shared_from_this());
}

uint32_t Object::getZIdx() const {
  return zIdx_;
}

DiscreteOrientation Object::getObjectOrientation() const {
  return orientation_;
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

Object::Object(std::string objectName, uint32_t id, uint32_t zIdx, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator) : objectName_(objectName), id_(id), zIdx_(zIdx), objectGenerator_(objectGenerator) {
  availableVariables.insert({"_x", x_});
  availableVariables.insert({"_y", y_});

  availableVariables_ = availableVariables;
}

Object::~Object() {}

}  // namespace griddly