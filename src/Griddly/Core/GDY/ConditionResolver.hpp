#pragma once
#include <yaml-cpp/yaml.h>

#include <functional>
#include <vector>

#include "YAMLUtils.hpp"

namespace griddly {

enum class LogicOp {
  NONE,
  AND,
  OR,
};

template <class ConditionFunction>
class ConditionResolver {
 public:
  ConditionFunction instantiateCondition(std::string &commandName, YAML::Node &conditionNode) const {
    if (commandName == "eq") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a == b; }, conditionNode);
    } else if (commandName == "gt") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a > b; }, conditionNode);
    } else if (commandName == "gte") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a >= b; }, conditionNode);
    } else if (commandName == "lt") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a < b; }, conditionNode);
    } else if (commandName == "lte") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a <= b; }, conditionNode);
    } else if (commandName == "neq") {
      return resolveConditionArguments([](int32_t a, int32_t b) { return a != b; }, conditionNode);
    } else if (commandName == "and") {
      return processConditions(conditionNode, false, LogicOp::AND);
    } else if (commandName == "or") {
      return processConditions(conditionNode, false, LogicOp::OR);
    } else {
      throw std::invalid_argument(fmt::format("Unknown or badly defined condition command {0}.", commandName));
    }
  }

  ConditionFunction processConditions(YAML::Node &conditionNodeList, bool isTopLevel = false, LogicOp op = LogicOp::NONE) const {
    // We should have a single item and not a list
    if (!conditionNodeList.IsDefined()) {
      auto line = conditionNodeList.Mark().line;
      auto errorString = fmt::format("Parse error line {0}. If statement is missing Conditions", line);
      spdlog::error(errorString);
      throw std::invalid_argument(errorString);
    }

    if (conditionNodeList.IsMap()) {
      if (conditionNodeList.size() != 1) {
        auto line = conditionNodeList.Mark().line;
        auto errorString = fmt::format("Parse error line {0}. Conditions must contain a single top-level condition", line);
        spdlog::error(errorString);
        throw std::invalid_argument(errorString);
      }
      auto conditionNode = conditionNodeList.begin();
      auto commandName = conditionNode->first.as<std::string>();
      return instantiateCondition(commandName, conditionNode->second);

    } else if (conditionNodeList.IsSequence()) {
      std::vector<ConditionFunction> conditionList;
      for (auto &&subConditionNode : conditionNodeList) {
        auto validatedNode = validateCommandPairNode(subConditionNode);
        auto commandName = validatedNode->first.as<std::string>();
        conditionList.push_back(instantiateCondition(commandName, validatedNode->second));
      }
      switch (op) {
        case LogicOp::AND:
          return resolveAND(conditionList);
        case LogicOp::OR:
          return resolveOR(conditionList);
        default: {
          auto line = conditionNodeList.Mark().line;
          auto errorString = fmt::format("Parse error line {0}. A sequence of conditions must be within an AND or an OR operator.", line);
          spdlog::error(errorString);
          throw std::invalid_argument(errorString);
        }
      }
    } else {
      auto line = conditionNodeList.Mark().line;
      auto errorString = fmt::format("Conditions must be a map or a list", line);
      spdlog::error(errorString);
      throw std::invalid_argument(errorString);
    }
  }

  virtual ConditionFunction resolveConditionArguments(const std::function<bool(int32_t, int32_t)> conditionFunction, YAML::Node &conditionArgumentsNode) const = 0;
  virtual ConditionFunction resolveAND(const std::vector<ConditionFunction> &conditionList) const = 0;
  virtual ConditionFunction resolveOR(const std::vector<ConditionFunction> &conditionList) const = 0;
};

}  // namespace griddly