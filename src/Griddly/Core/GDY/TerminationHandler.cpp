#include <spdlog/spdlog.h>
#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../Players/Player.hpp"
#include "../Util/util.hpp"
#include "TerminationHandler.hpp"

namespace griddly {

TerminationHandler::TerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) : grid_(grid), players_(players) {
  for (auto p : players) {
    auto scorePtr = p->getScore();
    auto playerId = p->getId();
    availableVariables_["_score"].insert({playerId, scorePtr});
  }

  availableVariables_["_steps"].insert({0, grid->getTickCount()});

  for (auto gVariable : grid->getGlobalVariables()) {
    auto variableArg = gVariable.first;
    auto variablePointer = gVariable.second;
    availableVariables_[variableArg].insert(variablePointer.begin(), variablePointer.end());
  }
}

TerminationHandler::~TerminationHandler() {
}

TerminationFunction TerminationHandler::instantiateTerminationCondition(TerminationState state, std::string commandName, uint32_t playerId, int32_t reward, int32_t opposingReward, std::vector<std::shared_ptr<int32_t>> variablePointers) {
  spdlog::debug("Adding termination condition={0} for player {1}", commandName, playerId);

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

  return [this, variablePointers, condition, playerId, state, reward, opposingReward, commandName]() {
    auto a = *(variablePointers[0]);
    auto b = *(variablePointers[1]);

    spdlog::debug("Checking condition {0} {1} {2}", a, commandName, b);

    if (condition(a, b)) {
      TerminationState oppositeState;
      if (playerId == 0) {
        oppositeState = state;
      } else {
        oppositeState = (state == TerminationState::WIN) ? TerminationState::LOSE : TerminationState::WIN;
      }

      std::unordered_map<uint32_t, TerminationState> playerTerminationStates;
      std::unordered_map<uint32_t, int32_t> playerTerminationRewards;
      for (auto p : players_) {
        auto pid = p->getId();
        if (pid == playerId || state == TerminationState::NONE) {
          playerTerminationStates[pid] = state;
          playerTerminationRewards[pid] = state == TerminationState::NONE ? 0 : reward;
        } else {
          playerTerminationStates[pid] = oppositeState;
          playerTerminationRewards[pid] = oppositeState == TerminationState::NONE ? 0 :opposingReward;
        }
      }

      return TerminationResult{
          true, playerTerminationRewards, playerTerminationStates};
    }

    return TerminationResult();
  };
}

void TerminationHandler::resolveTerminationConditions(TerminationState state, std::string commandName, int32_t reward, int32_t opposingReward, std::vector<std::string> terminationVariables) {
  // Termination variables grows with the number of players in the game
  auto resolvedVariableSets = findVariables(terminationVariables);

  spdlog::debug("Resolving termination condition {0} {1} {2}", terminationVariables[0], commandName, terminationVariables[1]);

  // Have to assume there are only two variables in these conditions
  std::unordered_map<uint32_t, std::vector<std::shared_ptr<int32_t>>> conditionArguments;

  for (auto lhs : resolvedVariableSets[0]) {
    auto lPlayerId = lhs.first;
    auto lVariable = lhs.second;
    for (auto rhs : resolvedVariableSets[1]) {
      auto rPlayerId = lhs.first;
      auto rVariable = rhs.second;
      conditionArguments[lPlayerId] = {lVariable, rVariable};
    }
  }


  for (auto resolvedTerminationCondition : conditionArguments) {
    auto playerId = resolvedTerminationCondition.first;
    auto resolvedVariables = resolvedTerminationCondition.second;
    if(conditionArguments.size() > 1 && playerId == 0) {
      continue;
    }
    terminationFunctions_.push_back(instantiateTerminationCondition(state, commandName, playerId, reward, opposingReward, resolvedVariables));
  }
}

void TerminationHandler::addTerminationCondition(TerminationConditionDefinition terminationConditionDefinition) {
  auto terminationState = terminationConditionDefinition.state;
  auto commandName = terminationConditionDefinition.commandName;
  auto commandArguments = terminationConditionDefinition.commandArguments;
  auto reward = terminationConditionDefinition.reward;
  auto opposingReward = terminationConditionDefinition.opposingReward;

  resolveTerminationConditions(terminationState, commandName, reward, opposingReward, commandArguments);
}

std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> TerminationHandler::findVariables(std::vector<std::string> variableArgs) {
  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolvedVariables;

  for (auto &variableArg : variableArgs) {
    auto variable = availableVariables_.find(variableArg);
    std::unordered_map<uint32_t, std::shared_ptr<int32_t>> resolvedVariable;

    // if we have a player variable, ignore the value with playerId 0 as it is "no player"
    // if(variable->second.size() > 1) {
    //   if(variable->second.find(0) != variable->second.end()) {
    //     variable->second.erase(0);
    //   }
    // }

    if (variable == availableVariables_.end()) {
      spdlog::debug("Global variable {0} not found, looking for player specific variables", variableArg);
      auto variableParts = split(variableArg, ':');
      if (variableParts.size() > 1) {
        auto objectName = variableParts[0];
        auto objectVariable = variableParts[1];
        spdlog::debug("Variable={0} for object={1} being resolved for each player.", objectVariable, objectName);

        if (objectVariable == "count") {
          resolvedVariable = grid_->getObjectCounter(objectName);
        } else {
          auto error = fmt::format("Undefined variable={0} or sub-variable={1}", objectName, objectVariable);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }

      } else {
        spdlog::debug("Variable string not found, trying to parse literal={0}", variableArg);

        try {
          resolvedVariable = {{0, std::make_shared<int32_t>(std::stoi(variableArg))}};
        } catch (const std::exception &e) {
          auto error = fmt::format("Undefined variable={0}", variableArg);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }
      }
    } else {
      spdlog::debug("Variable {0} resolved for players", variable->first);
      resolvedVariable = variable->second;
    }

    resolvedVariables.push_back(resolvedVariable);
  }

  return resolvedVariables;
}

TerminationResult TerminationHandler::isTerminated() {
  for (auto terminationFunction : terminationFunctions_) {
    auto terminationResult = terminationFunction();
    if (terminationResult.terminated) {
      return terminationResult;
    }
  }

  return TerminationResult();
}

}  // namespace griddly