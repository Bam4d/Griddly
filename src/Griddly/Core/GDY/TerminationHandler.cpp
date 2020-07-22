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

  availableVariables_["_max_steps"].insert({0, grid->getTickCount()});

  for (auto gVariable : grid->getGlobalVariables()) {
    auto variableArg = gVariable.first;
    auto variablePointer = gVariable.second;
    availableVariables_[variableArg].insert({0, variablePointer});
  }
}

TerminationHandler::~TerminationHandler() {
}

TerminationFunction TerminationHandler::instantiateTerminationCondition(TerminationState state, std::string commandName, uint32_t playerId, std::vector<std::shared_ptr<int32_t>> variablePointers) {
  spdlog::debug("Adding termination condition={0} for player {1}", commandName, playerId);

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

  return [this, variablePointers, condition, playerId, state, commandName]() {
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
      for (auto p : players_) {
        auto pid = p->getId();
        if (pid == playerId || state == TerminationState::NONE) {
          playerTerminationStates[pid] = state;
        } else {
          playerTerminationStates[pid] = oppositeState;
        }
      }

      return TerminationResult{
          true, playerTerminationStates};
    }

    return TerminationResult();
  };
}

void TerminationHandler::resolveTerminationConditions(TerminationState state, std::string commandName, std::vector<std::string> terminationVariables) {
  // Termination variables grows with the number of players in the game
  auto resolvedVariableSets = findVariables(terminationVariables);

  // Have to assume there are only two variables in these conditions
  std::unordered_map<uint32_t, std::vector<std::shared_ptr<int32_t>>> conditionArguments;

  for (auto lhs : resolvedVariableSets[0]) {
    auto lPlayerId = lhs.first;
    auto lVariable = lhs.second;
    for (auto rhs : resolvedVariableSets[1]) {
      auto rVariable = rhs.second;
      conditionArguments[lPlayerId] = {lVariable, rVariable};
    }
  }

  for (auto resolvedTerminationCondition : conditionArguments) {
    auto playerId = resolvedTerminationCondition.first;
    auto resolvedVariables = resolvedTerminationCondition.second;
    terminationFunctions_.push_back(instantiateTerminationCondition(state, commandName, playerId, resolvedVariables));
  }
}

void TerminationHandler::addTerminationCondition(TerminationConditionDefinition terminationConditionDefinition) {
  auto terminationState = terminationConditionDefinition.state;
  auto commandName = terminationConditionDefinition.commandName;
  auto commandArguments = terminationConditionDefinition.commandArguments;

  resolveTerminationConditions(terminationState, commandName, commandArguments);
}

std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> TerminationHandler::findVariables(std::vector<std::string> variableArgs) {
  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolvedVariables;

  for (auto &variableArg : variableArgs) {
    auto variable = availableVariables_.find(variableArg);
    std::unordered_map<uint32_t, std::shared_ptr<int32_t>> resolvedVariable;

    if (variable == availableVariables_.end()) {
      spdlog::debug("Global variable {0} not found, looking for player specific variables", variableArg);
      auto variableParts = split(variableArg, ':');
      if (variableParts.size() > 1) {
        auto objectName = variableParts[0];
        auto objectVariable = variableParts[1];
        spdlog::debug("Variableter={0} for object={1} being resolved for each player.", objectVariable, objectName);

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