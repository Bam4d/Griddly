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

  // used for generating random grid locations
  grid_location_width_distribution_ = std::uniform_int_distribution<uint32_t>(0, grid->getWidth()); 
  grid_location_height_distribution_ = std::uniform_int_distribution<uint32_t>(0, grid->getHeight()); 
}

uint32_t Object::getObjectId() const {
  return id_;
}

std::string Object::getDescription() const {
  return fmt::format("{0}@[{1}, {2}]", objectName_, *x_, *y_);
}

BehaviourResult Object::onActionSrc(std::string destinationObjectName, std::shared_ptr<Action> action) {
  auto actionName = action->getActionName();
  
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

BehaviourResult Object::onActionDst(std::shared_ptr<Action> action) {
  auto actionName = action->getActionName();
  auto sourceObject = action->getSourceObject();
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

std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> Object::resolveVariables(BehaviourCommandArguments commandArguments) {

  std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> resolvedVariables; 
  for(auto commandArgument : commandArguments) {
    resolvedVariables[commandArgument.first] = std::shared_ptr<ObjectVariable>(new ObjectVariable(commandArgument.second, availableVariables_));
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

  return [this, condition, a, b](std::shared_ptr<Action> action) {
    return condition(a->resolve(action), b->resolve(action));
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

  std::vector<BehaviourFunction> conditionalBehaviours;

  for (auto subCommand : subCommands) {
    auto subCommandName = subCommand.first;
    auto subCommandVariables = subCommand.second;

    conditionalBehaviours.push_back(instantiateBehaviour(subCommandName, subCommandVariables));
  }

  auto variablePointers = resolveVariables(commandArguments);
  auto a = variablePointers["0"];
  auto b = variablePointers["1"];

  return [this, condition, conditionalBehaviours, a, b](std::shared_ptr<Action> action) {
    if (condition(a->resolve(action), b->resolve(action))) {
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

    return BehaviourResult{false, 0};
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
  
  if (commandName == "change_to") {
    auto objectName = commandArguments["0"].as<std::string>();
    return [this, objectName](std::shared_ptr<Action> action) {
      spdlog::debug("Changing object={0} to {1}", getObjectName(), objectName);
      auto newObject = objectGenerator_->newInstance(objectName, grid_->getGlobalVariables());
      auto playerId = getPlayerId();
      auto location = getLocation();
      removeObject();
      grid_->addObject(playerId, location, newObject);
      return BehaviourResult();
    };
  }

  if (commandName == "add") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) {
      *a->resolve_ptr(action) += b->resolve(action);
      return BehaviourResult();
    };
  }

  if (commandName == "sub") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) {
      *a->resolve_ptr(action) -= b->resolve(action);
      return BehaviourResult();
    };
  }

  if (commandName == "set") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) {
      *a->resolve_ptr(action) = b->resolve(action);
      return BehaviourResult();
    };
  }

  if (commandName == "incr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) {
      (*a->resolve_ptr(action)) += 1;
      return BehaviourResult();
    };
  }
  
  if (commandName == "decr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) {
      (*a->resolve_ptr(action)) -= 1;
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
      auto objectMoved = moveObject({x->resolve(action), y->resolve(action)});
      return BehaviourResult{!objectMoved};
    };
  }
  
  if (commandName == "cascade") {
    auto a = commandArguments["0"].as<std::string>();
    return [this, a](std::shared_ptr<Action> action) {
      if (a == "_dest") {
        std::shared_ptr<Action> cascadedAction = std::shared_ptr<Action>(new Action(grid_, action->getActionName(), action->getDelay()));

        
        cascadedAction->init(action->getDestinationObject(), action->getVectorToDest(), action->getOrientationVector(), false);

        auto sourceLocation = cascadedAction->getSourceLocation();
        auto destinationLocation = cascadedAction->getDestinationLocation();
        auto vectorToDest = action->getVectorToDest();
        spdlog::debug("Cascade vector [{0},{1}]", vectorToDest.x, vectorToDest.y);
        spdlog::debug("Cascading action to [{0},{1}], dst: [{2}, {3}]", sourceLocation.x, sourceLocation.y, destinationLocation.x, destinationLocation.y );

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
    auto randomize = commandArguments["Randomize"].as<bool>(false);
    auto actionId = commandArguments["ActionId"].as<uint32_t>(0);

    // Resolve source object
    return [this, actionName, delay, randomize, actionId](std::shared_ptr<Action> action) {
      std::shared_ptr<Action> newAction = std::shared_ptr<Action>(new Action(grid_, actionName, delay));

      InputMapping fallbackInputMapping;
      fallbackInputMapping.vectorToDest = action->getVectorToDest();
      fallbackInputMapping.orientationVector = action->getOrientationVector();

      auto inputMapping = getInputMapping(actionName, actionId, randomize, fallbackInputMapping);

      newAction->init(shared_from_this(), inputMapping.vectorToDest, inputMapping.orientationVector, inputMapping.relative);
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
  
  
  if (commandName == "set_tile") {
    auto tileId = commandArguments["0"].as<uint32_t>();
    return [this, tileId](std::shared_ptr<Action> action) {
      setRenderTileId(tileId);
      return BehaviourResult();
    };
  } 
  
  if (commandName == "spawn") {
    auto objectName = commandArguments["0"].as<std::string>();
    return [this, objectName](std::shared_ptr<Action> action) {
      auto destinationLocation = action->getDestinationLocation();
      spdlog::debug("Spawning object={0} in location [{1},{2}]", objectName, destinationLocation.x, destinationLocation.y);
      auto newObject = objectGenerator_->newInstance(objectName, grid_->getGlobalVariables());
      auto playerId = getPlayerId();
      grid_->addObject(playerId, destinationLocation, newObject);
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

  // This object can perform this action
  availableActionNames_.insert(actionName);

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

bool Object::isValidAction(std::shared_ptr<Action> action) const {
  auto actionName = action->getActionName();
  auto destinationObject = action->getDestinationObject();
  auto destinationObjectName = destinationObject == nullptr ? "_empty" : destinationObject->getObjectName();

  spdlog::debug("Checking preconditions for action [{0}] -> {1} -> {2}", getObjectName(), actionName, destinationObjectName);

  // There are no source behaviours for this action, so this action cannot happen
  auto it = srcBehaviours_.find(actionName);
  if (it == srcBehaviours_.end()) {
    spdlog::debug("No source behaviours for action {0} on object {1}", actionName, objectName_);
    return false;
  }

  // Check the source behaviours against the destination object
  if(it->second.find(destinationObjectName) == it->second.end()) {
    spdlog::debug("No destination behaviours for object {0} performing action {1} on object {2}", objectName_, actionName, destinationObjectName);
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
    spdlog::debug("Precondition found, but not with destination object {0}. Passing.", destinationObjectName);
    return true;
  }

  auto &preconditions = preconditionsForActionAndDestinationObjectIt->second;

  for (auto precondition : preconditions) {
    if (!precondition(action)) {
      spdlog::debug("Precondition check failed for object {0} performing action {1} on object {2}", objectName_, actionName, destinationObjectName);
      return false;
    }
  }

  return true;
}

std::unordered_map<std::string, std::shared_ptr<int32_t>> Object::getAvailableVariables() const {
  return availableVariables_;
}

std::shared_ptr<int32_t> Object::getVariableValue(std::string variableName) {
  auto it = availableVariables_.find(variableName);
  if (it == availableVariables_.end()) {
    return nullptr;
  }

  return it->second;
}

SingleInputMapping Object::getInputMapping(std::string actionName, uint32_t actionId, bool randomize, InputMapping fallback) {
  auto actionInputsDefinitions = objectGenerator_->getActionInputDefinitions();
  auto actionInputsDefinitionIt = actionInputsDefinitions.find(actionName);

  if (actionInputsDefinitionIt == actionInputsDefinitions.end()) {
    auto error = fmt::format("Action {0} not found in input definitions.", actionName);
    throw std::runtime_error(error);
  }

  auto actionInputsDefinition = actionInputsDefinitionIt->second;
  auto inputMappings = actionInputsDefinition.inputMappings;

  SingleInputMapping resolvedInputMapping = {actionInputsDefinition.relative, actionInputsDefinition.internal, actionInputsDefinition.mappedToGrid};
  if(actionInputsDefinition.mappedToGrid) {

    if (randomize) {
      auto rand_x = grid_location_width_distribution_(random_generator_);
      auto rand_y = grid_location_height_distribution_(random_generator_);
      
      resolvedInputMapping.destinationLocation = {rand_x, rand_y};
    } else {

    }

  } else {

    InputMapping inputMapping;
    if (randomize) {
      auto it = inputMappings.begin();
      std::advance(it, rand() % inputMappings.size());
      inputMapping = it->second;
    } else if (actionId > 0) {
      auto it = inputMappings.find(actionId);
      if (it == inputMappings.end()) {
        auto error = fmt::format("Cannot find input mapping for action {0} with ActionId: {2}", actionName, actionId);
        throw std::runtime_error(error);
      }
      inputMapping = it->second;
    } else {
      inputMapping = fallback;
    }

    resolvedInputMapping.vectorToDest = inputMapping.vectorToDest;
    resolvedInputMapping.orientationVector = inputMapping.orientationVector;

  }

  return resolvedInputMapping;
}

void Object::setInitialActionDefinitions(std::vector<InitialActionDefinition> initialActionDefinitions) {
  initialActionDefinitions_ = initialActionDefinitions;
}

std::vector<std::shared_ptr<Action>> Object::getInitialActions() {
  std::vector<std::shared_ptr<Action>> initialActions;
  for (auto actionDefinition : initialActionDefinitions_) {
    auto actionInputsDefinitions = objectGenerator_->getActionInputDefinitions();
    auto actionInputsDefinition = actionInputsDefinitions[actionDefinition.actionName];

    auto inputMapping = getInputMapping(actionDefinition.actionName, actionDefinition.actionId, actionDefinition.randomize, InputMapping());

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

void Object::setRenderTileId(uint32_t renderTileId) {
  renderTileId_ = renderTileId;
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

std::string Object::getObjectRenderTileName() const {
  return objectName_ + std::to_string(renderTileId_);
}

bool Object::isPlayerAvatar() const {
  return isPlayerAvatar_;
}

void Object::markAsPlayerAvatar() {
  isPlayerAvatar_ = true;
}

std::unordered_set<std::string> Object::getAvailableActionNames() const {
  return availableActionNames_;
}

Object::Object(std::string objectName, uint32_t id, uint32_t zIdx, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator) : objectName_(objectName), id_(id), zIdx_(zIdx), objectGenerator_(objectGenerator) {
  availableVariables.insert({"_x", x_});
  availableVariables.insert({"_y", y_});

  availableVariables_ = availableVariables;
}

Object::~Object() {}

}  // namespace griddly