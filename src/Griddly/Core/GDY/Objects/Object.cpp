#include "Object.hpp"

#include <spdlog/spdlog.h>

#include <utility>

#include "../../AStarPathFinder.hpp"
#include "../../Grid.hpp"
#include "../../SpatialHashCollisionDetector.hpp"
#include "../../Util/util.hpp"
#include "../Actions/Action.hpp"
#include "ObjectGenerator.hpp"

namespace griddly {

Object::Object(std::string objectName, char mapCharacter, uint32_t playerId, uint32_t zIdx, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableVariables, std::shared_ptr<ObjectGenerator> objectGenerator, std::weak_ptr<Grid> grid)
    : objectName_(std::move(objectName)), mapCharacter_(mapCharacter), zIdx_(zIdx), objectGenerator_(std::move(objectGenerator)), grid_(std::move(grid)) {
  availableVariables.insert({"_x", x_});
  availableVariables.insert({"_y", y_});
  availableVariables.insert({"_dx", orientation_.getDx()});
  availableVariables.insert({"_dy", orientation_.getDy()});

  availableVariables.insert({"_playerId", playerId_});

  *playerId_ = playerId;

  availableVariables_ = availableVariables;
  renderTileName_ = objectName_ + std::to_string(renderTileId_);
}

Object::~Object() {
  spdlog::trace("Object Destroyed");
}

void Object::init(glm::ivec2 location) {
  init(location, DiscreteOrientation(Direction::NONE));
}

void Object::init(glm::ivec2 location, DiscreteOrientation orientation) {
  *x_ = location.x;
  *y_ = location.y;

  orientation_.setOrientation(orientation.getDirection());
  location_ = glm::ivec2(*x_, *y_);
}

const glm::ivec2 &Object::getLocation() const {
  return location_;
}

std::string Object::getDescription() const {
  return fmt::format("{0}@[{1}, {2}]", objectName_, location_.x, location_.y);
}

BehaviourResult Object::onActionSrc(std::string destinationObjectName, std::shared_ptr<Action> action, std::vector<uint32_t> behaviourIdxs) {
  const auto& actionName = action->getActionName();

  auto behavioursForActionIt = srcBehaviours_.find(actionName);
  if (behavioursForActionIt == srcBehaviours_.end()) {
    return {true};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(destinationObjectName);
  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    return {true};
  }

  spdlog::debug("Executing behaviours for source [{0}] -> {1} -> {2}", getObjectName(), actionName, destinationObjectName);
  auto &behaviours = behavioursForActionAndDestinationObject->second;

  std::unordered_map<uint32_t, int32_t> rewardAccumulator;
  for (const auto &idx : behaviourIdxs) {
    for (const auto &behaviour : behaviours.at(idx)) {
      auto result = behaviour(action);

      accumulateRewards(rewardAccumulator, result.rewards);
      if (result.abortAction) {
        return {true, rewardAccumulator};
      }
    }
  }

  return {false, rewardAccumulator};
}

BehaviourResult Object::onActionDst(std::shared_ptr<Action> action, std::vector<uint32_t> behaviourIdxs) {
  const auto& actionName = action->getActionName();
  auto sourceObject = action->getSourceObject();
  const auto sourceObjectName = sourceObject == nullptr ? "_empty" : sourceObject->getObjectName();

  auto behavioursForActionIt = dstBehaviours_.find(actionName);
  if (behavioursForActionIt == dstBehaviours_.end()) {
    spdlog::debug("Aborting dst behaviour, (no dst behaviours)", action->getDescription());
    return {true};
  }

  auto &behavioursForAction = behavioursForActionIt->second;

  auto behavioursForActionAndDestinationObject = behavioursForAction.find(sourceObjectName);

  if (behavioursForActionAndDestinationObject == behavioursForAction.end()) {
    spdlog::debug("Aborting dst behaviour, (no behaviours for action)", action->getDescription());
    return {true};
  }

  spdlog::debug("Executing behaviours for destination {0} -> {1} -> [{2}]", sourceObjectName, actionName, getObjectName());
  auto &behaviours = behavioursForActionAndDestinationObject->second;

  std::unordered_map<uint32_t, int32_t> rewardAccumulator;
  for (const auto &idx : behaviourIdxs) {
    for (const auto &behaviour : behaviours.at(idx)) {
      auto result = behaviour(action);

      accumulateRewards(rewardAccumulator, result.rewards);
      if (result.abortAction) {
        return {true, rewardAccumulator};
      }
    }
  }

  return {false, rewardAccumulator};
}

std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> Object::resolveVariables(CommandArguments &commandArguments, bool allowStrings) const {
  std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> resolvedVariables;
  for (auto commandArgument : commandArguments) {
    resolvedVariables[commandArgument.first] = std::make_shared<ObjectVariable>(ObjectVariable(commandArgument.second, availableVariables_, allowStrings));
  }

  return resolvedVariables;
}

BehaviourCondition Object::resolveConditionArguments(const std::function<bool(int32_t, int32_t)> condition, YAML::Node &conditionArgumentsNode) const {
  auto conditionArguments = singleOrListNodeToCommandArguments(conditionArgumentsNode);
  auto variablePointers = resolveVariables(conditionArguments);

  auto a = variablePointers["0"];
  auto b = variablePointers["1"];

  return [this, condition, a, b](std::shared_ptr<Action> action) {
    return condition(a->resolve(action), b->resolve(action));
  };
}

BehaviourCondition Object::resolveAND(const std::vector<BehaviourCondition> &conditionList) const {
  return [conditionList](const std::shared_ptr<Action> &action) -> bool {
    for (const auto &condition : conditionList) {
      if (!condition(action)) {
        return false;
      }
    }
    return true;
  };
}

BehaviourCondition Object::resolveOR(const std::vector<BehaviourCondition> &conditionList) const {
  return [conditionList](const std::shared_ptr<Action> &action) -> bool {
    for (const auto &condition : conditionList) {
      if (condition(action)) {
        return true;
      }
    }
    return false;
  };
}

BehaviourFunction Object::instantiateConditionalBehaviour(const std::string &commandName, CommandArguments &commandArguments, CommandList &subCommands) {
  if (subCommands.size() == 0) {
    return instantiateBehaviour(commandName, commandArguments);
  }

  std::function<bool(int32_t, int32_t)> condition;
  if (commandName == "eq") {
    condition = [](int32_t a, int32_t b) { return a == b; };
  } else if (commandName == "gt") {
    condition = [](int32_t a, int32_t b) { return a > b; };
  } else if (commandName == "gte") {
    condition = [](int32_t a, int32_t b) { return a >= b; };
  } else if (commandName == "lt") {
    condition = [](int32_t a, int32_t b) { return a < b; };
  } else if (commandName == "lte") {
    condition = [](int32_t a, int32_t b) { return a <= b; };
  } else if (commandName == "neq") {
    condition = [](int32_t a, int32_t b) { return a != b; };
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

  return [this, condition, conditionalBehaviours, a, b](const std::shared_ptr<Action> action) -> BehaviourResult {
    if (condition(a->resolve(action), b->resolve(action))) {
      std::unordered_map<uint32_t, int32_t> rewardAccumulator;
      return executeBehaviourFunctionList(rewardAccumulator, conditionalBehaviours, action);
    }

    return {};
  };
}

/**
 * @brief executes a list of behaviour functions and accumulates the rewards
 * 
 * @param rewardAccumulator 
 * @param behaviourList 
 * @param action 
 * @return BehaviourResult 
 */
BehaviourResult Object::executeBehaviourFunctionList(std::unordered_map<uint32_t, int32_t> &rewardAccumulator, const std::vector<BehaviourFunction> &behaviourList, const std::shared_ptr<Action> &action) const {
  for (auto &behaviour : behaviourList) {
    auto result = behaviour(action);

    accumulateRewards(rewardAccumulator, result.rewards);
    if (result.abortAction) {
      return {true, rewardAccumulator};
    }
  }

  return {false, rewardAccumulator};
}

BehaviourFunction Object::instantiateBehaviour(const std::string &commandName, CommandArguments &commandArguments) {
  // Command just used in tests
  if (commandName == "nop") {
    return [this](std::shared_ptr<Action> action) -> BehaviourResult {
      return {};
    };
  }

  if (commandName == "print") {
    auto resolvedVariables = resolveVariables(commandArguments, true);
    return [this, resolvedVariables](std::shared_ptr<Action> action) -> BehaviourResult {
      std::stringstream printline;

      for (const auto &resolvedVariable : resolvedVariables) {
        printline << " " << resolvedVariable.second->resolveString(action);
      }
      spdlog::info(printline.str());

      return {};
    };
  }

  if (commandName == "if") {
    auto conditions = getCommandArgument<YAML::Node>(commandArguments, "Conditions", YAML::Node(YAML::NodeType::Undefined));
    auto onTrueCommands = getCommandArgument<YAML::Node>(commandArguments, "OnTrue", YAML::Node(YAML::NodeType::Undefined));
    auto onFalseCommands = getCommandArgument<YAML::Node>(commandArguments, "OnFalse", YAML::Node(YAML::NodeType::Undefined));

    // For everthing in OnTrue:
    std::vector<BehaviourFunction> onTrueCommandList;

    if(onTrueCommands.IsDefined() && !onTrueCommands.IsSequence()) {
      auto line = onTrueCommands.Mark().line;
      auto errorString = fmt::format("Parse Error line {0}. Commands must be a list.", line);
      spdlog::error(errorString);
      throw std::invalid_argument(errorString);
    }

    if(onFalseCommands.IsDefined() && !onFalseCommands.IsSequence()) {
      auto line = onFalseCommands.Mark().line;
      auto errorString = fmt::format("Parse Error line {0}. Commands must be a list.", line);
      spdlog::error(errorString);
      throw std::invalid_argument(errorString);
    }

    for (auto &&conditionSubCommand : onTrueCommands) {

      auto subCommandIt = validateCommandPairNode(conditionSubCommand);
      auto subCommandName = subCommandIt->first.as<std::string>();
      auto subCommandArguments = subCommandIt->second;

      auto subCommandArgumentMap = singleOrListNodeToCommandArguments(subCommandArguments);

      onTrueCommandList.emplace_back(instantiateBehaviour(subCommandName, subCommandArgumentMap));
    }

    // For everything in OnFalse
    std::vector<BehaviourFunction> onFalseCommandList;
    for (auto &&conditionSubCommand : onFalseCommands) {
      auto subCommandIt = validateCommandPairNode(conditionSubCommand);
      auto subCommandName = subCommandIt->first.as<std::string>();
      auto subCommandArguments = subCommandIt->second;

      auto subCommandArgumentMap = singleOrListNodeToCommandArguments(subCommandArguments);

      onFalseCommandList.emplace_back(instantiateBehaviour(subCommandName, subCommandArgumentMap));
    }

    BehaviourCondition condition = processConditions(conditions, true, LogicOp::NONE);

    return [this, onTrueCommandList, onFalseCommandList, condition](const std::shared_ptr<Action> &action) -> BehaviourResult {
      std::unordered_map<uint32_t, int32_t> rewardAccumulator;
      if (condition(action)) {
        return executeBehaviourFunctionList(rewardAccumulator, onTrueCommandList, action);
      } else {
        return executeBehaviourFunctionList(rewardAccumulator, onFalseCommandList, action);
      }
    };
  }

  // reward the player that owns this particular object, otherwise warn
  if (commandName == "reward") {
    auto variablePointers = resolveVariables(commandArguments);
    auto value = variablePointers["0"];
    return [this, value](std::shared_ptr<Action> action) -> BehaviourResult {
      // if the object has a player Id, the reward will be given to that object's player,
      // otherwise the reward will be given to the player which has performed the action
      auto rewardPlayer = getPlayerId() == 0 ? action->getOriginatingPlayerId() : getPlayerId();

      if (rewardPlayer == 0) {
        spdlog::warn("Misconfigured 'reward' for object '{0}' will not be assigned to a player.", action->getSourceObject()->getDescription());
        return {};
      }

      // Find the player id of this object and give rewards to this player.
      return {false, {{rewardPlayer, value->resolve(action)}}};
    };
  }

  if (commandName == "change_to") {
    auto objectName = commandArguments["0"].as<std::string>();
    return [this, objectName](std::shared_ptr<Action> action) -> BehaviourResult {
      spdlog::debug("Changing object={0} to {1}", getObjectName(), objectName);
      auto playerId = getPlayerId();
      auto location = getLocation();
      auto newObject = objectGenerator_->newInstance(objectName, playerId, grid());
      removeObject();
      grid()->addObject(location, newObject, true, action);
      return {};
    };
  }

  if (commandName == "add") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) -> BehaviourResult {
      *a->resolve_ptr(action) += b->resolve(action);
      grid()->invalidateLocation(getLocation());
      return {};
    };
  }

  if (commandName == "sub") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) -> BehaviourResult {
      *a->resolve_ptr(action) -= b->resolve(action);
      grid()->invalidateLocation(getLocation());
      return {};
    };
  }

  if (commandName == "set") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    auto b = variablePointers["1"];
    return [this, a, b](std::shared_ptr<Action> action) -> BehaviourResult {
      spdlog::debug("set");
      *a->resolve_ptr(action) = b->resolve(action);
      grid()->invalidateLocation(getLocation());
      return {};
    };
  }

  if (commandName == "incr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) -> BehaviourResult {
      spdlog::debug("incr");
      (*a->resolve_ptr(action)) += 1;
      grid()->invalidateLocation(getLocation());
      return {};
    };
  }

  if (commandName == "decr") {
    auto variablePointers = resolveVariables(commandArguments);
    auto a = variablePointers["0"];
    return [this, a](std::shared_ptr<Action> action) -> BehaviourResult {
      spdlog::debug("decr");
      (*a->resolve_ptr(action)) -= 1;
      grid()->invalidateLocation(getLocation());
      return {};
    };
  }

  if (commandName == "rot") {
    if (commandArguments["0"].as<std::string>() == "_dir") {
      return [this](std::shared_ptr<Action> action) -> BehaviourResult {
        orientation_.setOrientation(action->getOrientationVector());

        // redraw the current location
        grid()->invalidateLocation(getLocation());
        return {};
      };
    }
  }

  if (commandName == "mov") {
    if (commandArguments["0"].as<std::string>() == "_dest") {
      return [this](std::shared_ptr<Action> action) -> BehaviourResult {
        auto objectMoved = moveObject(action->getDestinationLocation());
        return {!objectMoved};
      };
    }

    if (commandArguments["0"].as<std::string>() == "_src") {
      return [this](std::shared_ptr<Action> action) -> BehaviourResult {
        auto objectMoved = moveObject(action->getSourceLocation());
        return {!objectMoved};
      };
    }

    auto variablePointers = resolveVariables(commandArguments);

    if (variablePointers.size() != 2) {
      spdlog::error("Bad mov command detected! There should be two arguments but {0} were provided. This command will be ignored.", variablePointers.size());
      return [this](std::shared_ptr<Action> action) -> BehaviourResult {
        return {};
      };
    }

    auto x = variablePointers["0"];
    auto y = variablePointers["1"];

    return [this, x, y](std::shared_ptr<Action> action) -> BehaviourResult {
      auto objectMoved = moveObject({x->resolve(action), y->resolve(action)});
      return {!objectMoved};
    };
  }

  if (commandName == "cascade") {
    auto a = commandArguments["0"].as<std::string>();
    return [this, a](std::shared_ptr<Action> action) -> BehaviourResult {
      if (a == "_dest") {
        std::shared_ptr<Action> cascadedAction = std::make_shared<Action>(Action(grid(), action->getActionName(), action->getOriginatingPlayerId(), action->getDelay(), action->getMetaData()));

        cascadedAction->init(action->getDestinationObject(), action->getVectorToDest(), action->getOrientationVector(), false);

        auto sourceLocation = cascadedAction->getSourceLocation();
        auto destinationLocation = cascadedAction->getDestinationLocation();
        auto vectorToDest = action->getVectorToDest();
        spdlog::debug("Cascade vector [{0},{1}]", vectorToDest.x, vectorToDest.y);
        spdlog::debug("Cascading action to [{0},{1}], dst: [{2}, {3}]", sourceLocation.x, sourceLocation.y, destinationLocation.x, destinationLocation.y);

        auto actionRewards = grid()->performActions(0, {cascadedAction});

        return {false, actionRewards};
      }

      spdlog::warn("The only supported variable for cascade is _dest.");

      return {true};
    };
  }

  if (commandName == "exec") {
    auto actionName = getCommandArgument<std::string>(commandArguments, "Action", "");
    auto delayNode = getCommandArgument<YAML::Node>(commandArguments, "Delay", YAML::Node("0"));
    auto randomize = getCommandArgument<bool>(commandArguments, "Randomize", false);
    auto actionIdNode = getCommandArgument<YAML::Node>(commandArguments, "ActionId", YAML::Node("0"));
    auto executor = getCommandArgument<std::string>(commandArguments, "Executor", "action");
    auto searchNode = getCommandArgument<YAML::Node>(commandArguments, "Search", YAML::Node(YAML::NodeType::Undefined));
    auto resolvedExecMetaData = resolveActionMetaData(commandArguments);

    PathFinderConfig pathFinderConfig = configurePathFinder(searchNode, actionName);

    auto actionExecutor = getActionExecutorFromString(executor);

    auto delay = std::make_shared<ObjectVariable>(delayNode, availableVariables_);
    auto actionId = std::make_shared<ObjectVariable>(actionIdNode, availableVariables_);

    // Resolve source object
    return [this, actionName, delay, randomize, actionId, actionExecutor, resolvedExecMetaData, pathFinderConfig](std::shared_ptr<Action> action) -> BehaviourResult {
      InputMapping fallbackInputMapping;
      fallbackInputMapping.vectorToDest = action->getVectorToDest();
      fallbackInputMapping.orientationVector = action->getOrientationVector();

      // Resolve metaData variables if they exist
      for (const auto &resolvedExecMetaDataIt : resolvedExecMetaData) {
        fallbackInputMapping.metaData[resolvedExecMetaDataIt.first] = resolvedExecMetaDataIt.second->resolve(action);
      }

      SingleInputMapping inputMapping;
      if (pathFinderConfig.pathFinder != nullptr) {
        spdlog::debug("Executing action based on PathFinder");
        auto endLocation = pathFinderConfig.endLocation;
        if (pathFinderConfig.collisionDetector != nullptr) {
          auto searchResult = pathFinderConfig.collisionDetector->search(getLocation());

          if (searchResult.objectSet.empty()) {
            spdlog::debug("Cannot find target object for pathfinding!");
            return {};
          }

          endLocation = searchResult.closestObjects.top().target->getLocation();
        }

        spdlog::debug("Searching for path from [{0},{1}] to [{2},{3}] using action {4}", getLocation().x, getLocation().y, endLocation.x, endLocation.y, actionName);

        auto searchResult = pathFinderConfig.pathFinder->search(getLocation(), endLocation, getObjectOrientation().getUnitVector(), pathFinderConfig.maxSearchDepth);
        inputMapping = getInputMapping(actionName, searchResult.actionId, false, fallbackInputMapping);
      } else {
        inputMapping = getInputMapping(actionName, actionId->resolve(action), randomize, fallbackInputMapping);
      }

      if (inputMapping.mappedToGrid) {
        inputMapping.vectorToDest = inputMapping.destinationLocation - getLocation();
      }

      uint32_t execAsPlayerId = 0;
      switch (actionExecutor) {
        case ActionExecutor::ACTION_PLAYER_ID:
          execAsPlayerId = action->getOriginatingPlayerId();
          break;
        case ActionExecutor::OBJECT_PLAYER_ID:
          execAsPlayerId = getPlayerId();
          break;
        default:
          break;
      }

      std::shared_ptr<Action> newAction = std::make_shared<Action>(Action(grid(), actionName, execAsPlayerId, delay->resolve(action), inputMapping.metaData));
      newAction->init(shared_from_this(), inputMapping.vectorToDest, inputMapping.orientationVector, inputMapping.relative);

      auto rewards = grid()->performActions(0, {newAction});

      return {false, rewards};
    };
  }

  if (commandName == "remove") {
    return [this](std::shared_ptr<Action> action) -> BehaviourResult {
      spdlog::debug("remove");
      removeObject();
      return {};
    };
  }

  if (commandName == "set_tile") {
    auto variablePointers = resolveVariables(commandArguments);

    auto tileId = variablePointers["0"];

    return [this, tileId](std::shared_ptr<Action> action) -> BehaviourResult {
      auto resolvedTileId = tileId->resolve(action);
      spdlog::debug("Setting tile Id to: {0}", resolvedTileId);
      setRenderTileId(resolvedTileId);
      grid()->invalidateLocation({*x_, *y_});
      spdlog::debug("Tile id updated");
      return {};
    };
  }

  if (commandName == "spawn") {
    auto objectName = commandArguments["0"].as<std::string>();
    return [this, objectName](std::shared_ptr<Action> action) -> BehaviourResult {
      auto destinationLocation = action->getDestinationLocation();
      spdlog::debug("Spawning object={0} in location [{1},{2}]", objectName, destinationLocation.x, destinationLocation.y);
      auto playerId = getPlayerId();

      auto newObject = objectGenerator_->newInstance(objectName, playerId, grid());
      grid()->addObject(destinationLocation, newObject, true, action);
      return {};
    };
  }

  throw std::invalid_argument(fmt::format("Unknown or badly defined command {0}.", commandName));
}

void Object::addPrecondition(const std::string &actionName, uint32_t behaviourIdx, const std::string &destinationObjectName, YAML::Node &conditionsNode) {
  spdlog::debug("Adding action precondition when action={0} is performed on object={1} by object={2}", actionName, destinationObjectName, getObjectName());
  auto preconditionFunction = processConditions(conditionsNode, true, LogicOp::AND);
  actionPreconditions_[actionName][destinationObjectName][behaviourIdx] = preconditionFunction;
}

void Object::addActionSrcBehaviour(
    const std::string &actionName,
    uint32_t behaviourIdx,
    const std::string &destinationObjectName,
    const std::string &commandName,
    CommandArguments commandArguments,
    CommandList conditionalCommands) {
  spdlog::debug("Adding behaviour command={0} when action={1} is performed on object={2} by object={3}", commandName, actionName, destinationObjectName, getObjectName());

  // This object can perform this action
  availableActionNames_.insert(actionName);

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandArguments, conditionalCommands);
  srcBehaviours_[actionName][destinationObjectName][behaviourIdx].push_back(behaviourFunction);
}

void Object::addActionDstBehaviour(
    const std::string &actionName,
    uint32_t behaviourIdx,
    const std::string &sourceObjectName,
    const std::string &commandName,
    CommandArguments commandArguments,
    CommandList conditionalCommands) {
  spdlog::debug("Adding behaviour command={0} when object={1} performs action={2} on object={3}", commandName, sourceObjectName, actionName, getObjectName());

  auto behaviourFunction = instantiateConditionalBehaviour(commandName, commandArguments, conditionalCommands);
  dstBehaviours_[actionName][sourceObjectName][behaviourIdx].push_back(behaviourFunction);
}

std::vector<uint32_t> Object::getValidBehaviourIdxs(std::shared_ptr<Action> action) const {
  std::vector<uint32_t> validBehaviourIdxs{};
  const auto& actionName = action->getActionName();
  auto destinationObject = action->getDestinationObject();

  std::string destinationObjectName = destinationObject->getObjectName();
  if (destinationObjectName == "_empty") {
    auto width = grid()->getWidth();
    auto height = grid()->getHeight();


    // Check that the destination of the action is not outside the grid
    auto destinationLocation = action->getDestinationLocation();
    if (destinationLocation.x >= width || destinationLocation.x < 0 ||
        destinationLocation.y >= height || destinationLocation.y < 0) {
      destinationObjectName = "_boundary";
    }
  }

  spdlog::debug("Checking preconditions for action [{0}] -> {1} -> {2}", getObjectName(), actionName, destinationObjectName);

  // There are no source behaviours for this action, so this action cannot happen
  auto it = srcBehaviours_.find(actionName);
  if (it == srcBehaviours_.end()) {
    spdlog::debug("No source behaviours for action {0} on object {1}", actionName, objectName_);
    return {};
  }

  // Check the source behaviours against the destination object
  const auto &destBehavioursIt = it->second.find(destinationObjectName);
  if (destBehavioursIt == it->second.end()) {
    spdlog::debug("No destination behaviours for object {0} performing action {1} on object {2}", objectName_, actionName, destinationObjectName);
    return {};
  }

  // Check for preconditions
  auto preconditionsForActionIt = actionPreconditions_.find(actionName);

  // If there are no preconditions then we just let the action happen
  if (preconditionsForActionIt == actionPreconditions_.end()) {
    spdlog::debug("No preconditions found, returning all {0} possible actions", destBehavioursIt->second.size());
    for (const auto behaviourIdx : destBehavioursIt->second) {
      validBehaviourIdxs.push_back(behaviourIdx.first);
    }

    return validBehaviourIdxs;
  }

  auto &preconditionsForAction = preconditionsForActionIt->second;
  spdlog::debug("{0} preconditions found.", preconditionsForAction.size());

  auto preconditionsForActionAndDestinationObjectIt = preconditionsForAction.find(destinationObjectName);
  if (preconditionsForActionAndDestinationObjectIt == preconditionsForAction.end()) {
    spdlog::debug("Precondition found, but not with destination object {0}. Passing.", destinationObjectName);
    for (const auto behaviourIdx : destBehavioursIt->second) {
      validBehaviourIdxs.push_back(behaviourIdx.first);
    }

    return validBehaviourIdxs;
  }

  auto &preconditions = preconditionsForActionAndDestinationObjectIt->second;

  for (const auto &behaviourPreconditionIt : preconditions) {
    if (behaviourPreconditionIt.second(action)) {
      validBehaviourIdxs.push_back(behaviourPreconditionIt.first);
    }
  }

  return validBehaviourIdxs;
}

bool Object::isValidAction(std::shared_ptr<Action> action) const {
  if (getValidBehaviourIdxs(action).size() > 0) {
    return true;
  } 
  return false;
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

SingleInputMapping Object::getInputMapping(const std::string &actionName, uint32_t actionId, bool randomize, InputMapping fallback) {
  const auto &actionInputsDefinitions = objectGenerator_->getActionInputDefinitions();

  if (actionInputsDefinitions.find(actionName) == actionInputsDefinitions.end()) {
    auto error = fmt::format("Action {0} not found in input definitions.", actionName);
    throw std::runtime_error(error);
  }

  const auto &actionInputsDefinition = actionInputsDefinitions.at(actionName);
  const auto &inputMappings = actionInputsDefinition.inputMappings;

  SingleInputMapping resolvedInputMapping = {actionInputsDefinition.relative, actionInputsDefinition.internal, actionInputsDefinition.mapToGrid};

  auto randomGenerator = grid()->getRandomGenerator();

  if (actionInputsDefinition.mapToGrid) {
    spdlog::debug("Getting mapped to grid mapping for action {0}", actionName);

    auto rand_x = randomGenerator->sampleInt(0, grid()->getWidth() - 1);
    auto rand_y = randomGenerator->sampleInt(0, grid()->getHeight() - 1);

    resolvedInputMapping.destinationLocation = {rand_x, rand_y};

  } else {
    spdlog::debug("Getting standard input mapping for action {0}", actionName);
    InputMapping inputMapping;
    if (randomize) {
      auto it = inputMappings.begin();
      auto sampledIdx = randomGenerator->sampleInt(0, inputMappings.size() - 1);
      std::advance(it, sampledIdx);
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
    resolvedInputMapping.metaData = inputMapping.metaData;
  }

  // Add metadata keys from fallback, only if they are not aleady present
  for (const auto &metaDataIt : fallback.metaData) {
    auto key = metaDataIt.first;
    if (resolvedInputMapping.metaData.find(key) == resolvedInputMapping.metaData.end()) {
      resolvedInputMapping.metaData[key] = metaDataIt.second;
    }
  }

  return resolvedInputMapping;
}

void Object::setInitialActionDefinitions(std::vector<InitialActionDefinition> initialActionDefinitions) {
  initialActionDefinitions_ = initialActionDefinitions;
}

std::vector<std::shared_ptr<Action>> Object::getInitialActions(std::shared_ptr<Action> originatingAction = nullptr) {
  std::vector<std::shared_ptr<Action>> initialActions;

  InputMapping fallbackInputMapping;
  if (originatingAction != nullptr) {
    fallbackInputMapping.vectorToDest = originatingAction->getVectorToDest();
    fallbackInputMapping.orientationVector = originatingAction->getOrientationVector();
    fallbackInputMapping.metaData = originatingAction->getMetaData();
  }

  for (auto actionDefinition : initialActionDefinitions_) {
    const auto &actionInputsDefinitions = objectGenerator_->getActionInputDefinitions();
    const auto &actionInputsDefinition = actionInputsDefinitions.at(actionDefinition.actionName);

    auto inputMapping = getInputMapping(actionDefinition.actionName, actionDefinition.actionId, actionDefinition.randomize, fallbackInputMapping);

    auto action = std::make_shared<Action>(Action(grid(), actionDefinition.actionName, 0, actionDefinition.delay, inputMapping.metaData));
    if (inputMapping.mappedToGrid) {
      inputMapping.vectorToDest = inputMapping.destinationLocation - getLocation();
    }

    action->init(shared_from_this(), inputMapping.vectorToDest, inputMapping.orientationVector, actionInputsDefinition.relative);

    initialActions.push_back(action);
  }

  return initialActions;
}

template <typename C>
C Object::getCommandArgument(CommandArguments &commandArguments, std::string commandArgumentKey, C defaultValue) {
  auto commandArgumentIt = commandArguments.find(commandArgumentKey);
  if (commandArgumentIt == commandArguments.end()) {
    return defaultValue;
  }

  return commandArgumentIt->second.as<C>(defaultValue);
}

std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> Object::resolveActionMetaData(CommandArguments &commandArguments) {
  auto commandArgumentIt = commandArguments.find("MetaData");
  if (commandArgumentIt == commandArguments.end()) {
    return {};
  }

  std::unordered_map<std::string, std::shared_ptr<ObjectVariable>> resolvedMetaData{};
  auto metaDataNode = commandArguments.at("MetaData");
  if (metaDataNode.IsDefined()) {
    for (YAML::const_iterator it = metaDataNode.begin(); it != metaDataNode.end(); ++it) {
      auto key = it->first.as<std::string>();
      resolvedMetaData[key] = std::make_shared<ObjectVariable>(ObjectVariable(it->second, availableVariables_));
    }
  }

  return resolvedMetaData;
}

PathFinderConfig Object::configurePathFinder(YAML::Node &searchNode, std::string actionName) {
  PathFinderConfig config;
  if (searchNode.IsDefined()) {
    spdlog::debug("Configuring path finder for action {0}", actionName);

    auto targetObjectNameNode = searchNode["TargetObjectName"];

    if (targetObjectNameNode.IsDefined()) {
      auto targetObjectName = targetObjectNameNode.as<std::string>();

      spdlog::debug("Path finder target object: {0}", targetObjectName);

      spdlog::debug("Grid height: {0}", grid()->getHeight());

      // Just make the range really large so we always look in all cells
      auto range = std::max(grid()->getWidth(), grid()->getHeight());

      config.collisionDetector = std::make_shared<SpatialHashCollisionDetector>(SpatialHashCollisionDetector(grid()->getWidth(), grid()->getHeight(), 10, range, TriggerType::RANGE_BOX_AREA));

      if (config.collisionDetector != nullptr) {
        grid()->addCollisionDetector({targetObjectName}, actionName + generateRandomString(5), config.collisionDetector);
      }
    }

    auto impassableObjectsList = singleOrListNodeToList(searchNode["ImpassableObjects"]);

    std::set<std::string> impassableObjectsSet(impassableObjectsList.begin(), impassableObjectsList.end());
    const auto &actionInputDefinitions = objectGenerator_->getActionInputDefinitions();
    auto actionInputDefinitionIt = actionInputDefinitions.find(actionName);

    config.maxSearchDepth = searchNode["MaxDepth"].as<uint32_t>(100);
    config.pathFinder = std::make_shared<AStarPathFinder>(AStarPathFinder(grid(), impassableObjectsSet, actionInputDefinitionIt->second));

    if (searchNode["TargetLocation"].IsDefined()) {
      auto targetEndLocation = singleOrListNodeToList<uint32_t>(searchNode["TargetLocation"]);
      config.endLocation = glm::ivec2(targetEndLocation[0], targetEndLocation[1]);
    }
  }

  return config;
}

uint32_t Object::getPlayerId() const {
  return *playerId_;
}

bool Object::moveObject(glm::ivec2 newLocation) {
  if (grid()->updateLocation(shared_from_this(), {*x_, *y_}, newLocation)) {
    *x_ = newLocation.x;
    *y_ = newLocation.y;
    location_ = glm::ivec2(*x_, *y_);
    return true;
  }

  return false;
}

void Object::setRenderTileId(uint32_t renderTileId) {
  renderTileId_ = renderTileId;
  renderTileName_ = objectName_ + std::to_string(renderTileId_);
}

uint32_t Object::getRenderTileId() const {
  return renderTileId_;
}

void Object::removeObject() {
  removed_ = true;
  grid()->removeObject(shared_from_this());
}

int32_t Object::getZIdx() const {
  return zIdx_;
}

DiscreteOrientation Object::getObjectOrientation() const {
  return orientation_;
}

const std::string &Object::getObjectName() const {
  return objectName_;
}

char Object::getMapCharacter() const {
  return mapCharacter_;
}

const std::string &Object::getObjectRenderTileName() const {
  return renderTileName_;
}

bool Object::isRemoved() const {
  return removed_;
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

std::shared_ptr<Grid> Object::grid() const {
  return grid_.lock();
}

ActionExecutor Object::getActionExecutorFromString(std::string executorString) const {
  if (executorString == "action") {
    return ActionExecutor::ACTION_PLAYER_ID;
  } else if (executorString == "object") {
    return ActionExecutor::OBJECT_PLAYER_ID;
  } else {
    auto errorString = fmt::format("Invalid Action Executor choice '{0}'.", executorString);
    spdlog::error(errorString);
    throw std::invalid_argument(errorString);
  }
}

}  // namespace griddly