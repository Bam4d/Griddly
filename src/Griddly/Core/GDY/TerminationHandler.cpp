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

TerminationFunction TerminationHandler::resolveConditionArguments(const std::function<bool(int32_t, int32_t)> conditionFunction, YAML::Node &conditionArgumentsNode) const {
  auto conditionArguments = singleOrListNodeToCommandArguments(conditionArgumentsNode);
  auto resolvedVariableSets = resolveVariables(conditionArguments);

  // we have to calculate the condition for each player
  std::unordered_map<uint32_t, std::vector<std::shared_ptr<int32_t>>> playerConditionArguments;
  for (auto lhs : resolvedVariableSets[0]) {
    auto lPlayerId = lhs.first;
    auto lVariable = lhs.second;
    for (auto rhs : resolvedVariableSets[1]) {
      auto rVariable = rhs.second;
      playerConditionArguments[lPlayerId] = {lVariable, rVariable};
    }
  }

  return [this, playerConditionArguments, conditionFunction]() -> std::unordered_map<uint32_t, bool> {
    std::unordered_map<uint32_t, bool> playerResults;
    for (auto resolvedTerminationCondition : playerConditionArguments) {
      auto playerId = resolvedTerminationCondition.first;
      auto resolvedVariables = resolvedTerminationCondition.second;
      if (playerConditionArguments.size() > 1 && playerId == 0) {
        playerResults[playerId] = false;
      } else {
        auto a = *(resolvedVariables[0]);
        auto b = *(resolvedVariables[1]);

        playerResults[playerId] = conditionFunction(a, b);
      }
    }

    return playerResults;
  };
}

TerminationFunction TerminationHandler::resolveAND(const std::vector<TerminationFunction> &conditionList) const {
  return [conditionList]() -> std::unordered_map<uint32_t, bool> {
    std::unordered_map<uint32_t, bool> playerANDResults;
    for (const auto &condition : conditionList) {
      for (const auto &playerResultIt : condition()) {
        auto playerId = playerResultIt.first;
        auto playerResult = playerResultIt.second;

        if (playerANDResults.find(playerId) == playerANDResults.end()) {
          playerANDResults[playerId] = playerResult;
        } else {
          playerANDResults[playerId] &= playerResult;
        }
      }
    }
    return playerANDResults;
  };
}

TerminationFunction TerminationHandler::resolveOR(const std::vector<TerminationFunction> &conditionList) const {
  return [conditionList]() -> std::unordered_map<uint32_t, bool> {
    std::unordered_map<uint32_t, bool> playerORResults;
    for (const auto &condition : conditionList) {
      for (const auto &playerResultIt : condition()) {
        auto playerId = playerResultIt.first;
        auto playerResult = playerResultIt.second;

        if (playerORResults.find(playerId) == playerORResults.end()) {
          playerORResults[playerId] = playerResult;
        } else {
          playerORResults[playerId] |= playerResult;
        }
      }
    }
    return playerORResults;
  };
}

void TerminationHandler::addTerminationCondition(TerminationConditionDefinition &terminationConditionDefinition) {
  resolvedTerminationConditions_.push_back({terminationConditionDefinition, processConditions(terminationConditionDefinition.conditionsNode, true, LogicOp::OR)});
}

std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> TerminationHandler::resolveVariables(CommandArguments &commandArguments) const {
  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolvedVariables;

  for (const auto &variableIt : commandArguments) {
    auto variableStr = variableIt.second.as<std::string>();
    auto variable = availableVariables_.find(variableStr);
    spdlog::debug("Parsing condition {0}", variableIt.first);
    std::unordered_map<uint32_t, std::shared_ptr<int32_t>> resolvedVariable;

    if (variable == availableVariables_.end()) {
      spdlog::debug("Global variable {0} not found, looking for player specific variables", variableStr);
      auto variableParts = split(variableStr, ':');
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
        spdlog::debug("Variable string not found, trying to parse literal={0}", variableStr);

        try {
          resolvedVariable.insert({0, std::make_shared<int32_t>(std::stoi(variableStr))});
        } catch (const std::exception &e) {
          auto error = fmt::format("Undefined variable={0}", variableStr);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }
      }
    } else {
      spdlog::debug("Variable {0} resolved for {1} players", variable->first, variable->second.size());
      resolvedVariable = variable->second;
    }

    // If we have a global variable or a literal, copy them for all players
    if (resolvedVariable.size() == 1) {
      auto resolvedVariableVal = resolvedVariable.at(0);
      for (const auto &player : players_) {
        resolvedVariable.insert({player->getId(), resolvedVariableVal});
      }
    }

    resolvedVariables.push_back(resolvedVariable);
  }

  return resolvedVariables;
}

TerminationResult TerminationHandler::isTerminated() {
  spdlog::debug("Checking for termination conditions");
  for (auto resolvedTerminationCondition : resolvedTerminationConditions_) {
    auto terminationResult = resolvedTerminationCondition.conditionFunction();

    bool isTerminated = false;
    for (const auto &playerResult : terminationResult) {
      if (playerResult.second) {
        isTerminated = true;
        break;
      }
    }

    if (isTerminated) {
      auto definitionState = resolvedTerminationCondition.definition.state;
      auto definitionReward = resolvedTerminationCondition.definition.reward;
      auto definitionOpposingReward = resolvedTerminationCondition.definition.opposingReward;

      std::unordered_map<uint32_t, TerminationState> playerTerminationStates;
      std::unordered_map<uint32_t, int32_t> playerTerminationRewards;
      for (const auto &playerResult : terminationResult) {
        auto playerId = playerResult.first;
        auto playerTerminated = playerResult.second;

        if (playerId > 0) {
          auto oppositeState = (definitionState == TerminationState::WIN) ? TerminationState::LOSE : TerminationState::WIN;

          if (playerTerminated || definitionState == TerminationState::NONE) {
            playerTerminationStates[playerId] = definitionState;
            playerTerminationRewards[playerId] = definitionState == TerminationState::NONE ? 0 : definitionReward;
          } else {
            playerTerminationStates[playerId] = oppositeState;
            playerTerminationRewards[playerId] = oppositeState == TerminationState::NONE ? 0 : definitionOpposingReward;
          }

          spdlog::debug("Player {0} termination state: {1}", playerId, getTerminationStateString(playerTerminationStates[playerId]));
          spdlog::debug("Player {0} reward: {1}", playerId, playerTerminationRewards[playerId]);
        }
      }

      return TerminationResult{
          true, playerTerminationRewards, playerTerminationStates};
    }
  }
  return TerminationResult();
}

}  // namespace griddly