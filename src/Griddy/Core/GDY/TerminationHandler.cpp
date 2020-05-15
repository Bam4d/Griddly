#include <spdlog/spdlog.h>
#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../Players/Player.hpp"
#include "../Util/util.hpp"
#include "TerminationHandler.hpp"

namespace griddy {

TerminationHandler::TerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) : grid_(grid), players_(players) {
  for (auto p : players) {
    auto scorePtr = p->getScore();
    auto playerId = p->getId();
    availableParameters_["_score"].insert({playerId, scorePtr});
  }

  availableParameters_["_max_steps"].insert({0, grid->getTickCount()});

  for (auto gParam : grid->getGlobalParameters()) {
    auto parameterName = gParam.first;
    auto parameterPointer = gParam.second;
    availableParameters_[parameterName].insert({0, parameterPointer});
  }
}

TerminationHandler::~TerminationHandler() {
}

TerminationFunction TerminationHandler::instantiateTerminationCondition(TerminationState state, std::string commandName, uint32_t playerId, std::vector<std::shared_ptr<int32_t>> parameterPointers) {
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

  return [this, parameterPointers, condition, playerId, state, commandName]() {
    auto a = *(parameterPointers[0]);
    auto b = *(parameterPointers[1]);

    spdlog::debug("Checking condition {0} {1} {2}", a, commandName, b);

    if (condition(a, b)) {

      auto oppositeState = (state == TerminationState::WIN) ? TerminationState::LOSE : TerminationState::WIN;

      std::unordered_map<uint32_t, TerminationState> playerTerminationStates;
      for(auto p : players_) {
        auto pid = p->getId();
        if(pid == playerId || state == TerminationState::NONE) {
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

void TerminationHandler::resolveTerminationConditions(TerminationState state, std::string commandName, std::vector<std::string> terminationParameters) {
  // Termination parameters grows with the number of players in the game
  auto resolvedParameterSets = findParameters(terminationParameters);

  // Have to assume there are only two parameters in these conditions
  std::unordered_map<uint32_t, std::vector<std::shared_ptr<int32_t>>> conditionArguments;

  for (auto lhs : resolvedParameterSets[0]) {
    auto lPlayerId = lhs.first;
    auto lParam = lhs.second;
    for (auto rhs : resolvedParameterSets[1]) {
      auto rParam = rhs.second;
      conditionArguments[lPlayerId] = {lParam, rParam};
    }
  }

  for (auto resolvedTerminationCondition : conditionArguments) {
    auto playerId = resolvedTerminationCondition.first;
    auto resolvedParameters = resolvedTerminationCondition.second;
    terminationFunctions_.push_back(instantiateTerminationCondition(state, commandName, playerId, resolvedParameters));
  }
}

void TerminationHandler::addTerminationCondition(TerminationConditionDefinition terminationConditionDefinition) {
  auto terminationState = terminationConditionDefinition.state;
  auto commandName = terminationConditionDefinition.commandName;
  auto commandParameters = terminationConditionDefinition.commandParameters;

  resolveTerminationConditions(terminationState, commandName, commandParameters);
}

std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> TerminationHandler::findParameters(std::vector<std::string> parameters) {
  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolvedParams;

  for (auto &param : parameters) {
    auto parameter = availableParameters_.find(param);
    std::unordered_map<uint32_t, std::shared_ptr<int32_t>> resolvedParam;

    if (parameter == availableParameters_.end()) {
      spdlog::debug("Global parameter string not found, looking for player specific parameters", param);
      auto paramParts = split(param, ':');
      if (paramParts.size() > 1) {
        auto objectName = paramParts[0];
        auto objectParam = paramParts[1];
        spdlog::debug("Paramter={0} for object={1} being resolved for each player.", objectParam, objectName);

        if (objectParam == "count") {
          resolvedParam = grid_->getObjectCounter(objectName);
        } else {
          auto error = fmt::format("Undefined parameter={0} or sub-parameter={1}", objectName, objectParam);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }

      } else {
        spdlog::debug("Parameter string not found, trying to parse literal={0}", param);

        try {
          resolvedParam = {{0, std::make_shared<int32_t>(std::stoi(param))}};
        } catch (const std::exception &e) {
          auto error = fmt::format("Undefined parameter={0}", param);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }
      }
    } else {
      spdlog::debug("Parameter {0} resolved for players", parameter->first);
      resolvedParam = parameter->second;
    }

    resolvedParams.push_back(resolvedParam);
  }

  return resolvedParams;
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

}  // namespace griddy