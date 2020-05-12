#include <spdlog/spdlog.h>
#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "TerminationHandler.hpp"
#include "../Util/util.hpp"

namespace griddy {

TerminationHandler::TerminationHandler(std::shared_ptr<Grid> grid) : grid_(grid) {
}

TerminationHandler::~TerminationHandler() {
}

TerminationFunction TerminationHandler::instantiateTerminationCondition(std::string commandName, uint32_t playerId, std::vector<std::shared_ptr<int32_t>> parameterPointers) {
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

  return [this, parameterPointers, condition, playerId]() {
    auto a = *(parameterPointers[0]);
    auto b = *(parameterPointers[1]);

    if (condition(a, b)) {
      return TerminationResult{
          true, playerId};
    }

    return TerminationResult();
  };
}

void TerminationHandler::resolveTerminationConditions(std::string commandName, std::vector<std::string> terminationParameters) {
  // Termination parameters grows with the number of players in the game
  auto resolvedParameterSets = findParameters(terminationParameters);

  std::unordered_map<uint32_t, std::vector<std::shared_ptr<int32_t>>> conditionArguments;
  for (auto resolvedParameterSet : resolvedParameterSets) {
    for (auto playerParameter : resolvedParameterSet) {
      auto playerId = playerParameter.first;
      auto parameter = playerParameter.second;
      conditionArguments[playerId].push_back(parameter);
    }
  }

  for (auto resolvedTerminationCondition : conditionArguments) {
    auto playerId = resolvedTerminationCondition.first;
    auto resolvedParameters = resolvedTerminationCondition.second;
    terminationFunctions_.push_back(instantiateTerminationCondition(commandName, playerId, resolvedParameters));
  }
}

void TerminationHandler::addTerminationCondition(std::pair<std::string, std::vector<std::string>> terminationConditionDefinition) {
  auto commandName = terminationConditionDefinition.first;
  auto commandParameters = terminationConditionDefinition.second;

  resolveTerminationConditions(commandName, commandParameters);
}

std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> TerminationHandler::findParameters(std::vector<std::string> parameters) {
  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolvedParams;

  for (auto &param : parameters) {
    auto parameter = availableParameters_.find(param);
    std::unordered_map<uint32_t, std::shared_ptr<int32_t>> resolvedParam;

    if (parameter == availableParameters_.end()) {
      spdlog::debug("Global parameter string not found, looking for player specific parameters", param);
      auto paramParts = split(param, ':');
      if (paramParts.size() > 0) {
        auto objectName = paramParts[0];
        auto objectParam = paramParts[1];
        spdlog::debug("Paramter={1} for object={0} being resolved for each player.", objectParam, objectName);

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
      resolvedParam = {{0, parameter->second}};
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