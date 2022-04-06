#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Grid.hpp"
#include "ConditionResolver.hpp"

#define TerminationFunction std::function<std::unordered_map<uint32_t, bool>()>

namespace griddly {

class Player;

enum class TerminationState {
  WIN,
  LOSE,
  NONE  // There does not have to be a winner or loser, just terminate
};

struct TerminationResult {
  bool terminated = false;
  std::unordered_map<uint32_t, int32_t> rewards{};
  std::unordered_map<uint32_t, TerminationState> playerStates{};
};


struct TerminationConditionDefinition {
  TerminationState state = TerminationState::NONE;
  int32_t reward = 0;
  int32_t opposingReward = 0;
  YAML::Node conditionsNode = YAML::Node(YAML::NodeType::Undefined);
};

struct ResolvedTerminationCondition {
  TerminationConditionDefinition definition{};
  TerminationFunction conditionFunction;
};

class TerminationHandler : ConditionResolver<TerminationFunction> {
 public:
  TerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players);
  virtual ~TerminationHandler() = default;
  virtual TerminationResult isTerminated();

  virtual void addTerminationCondition(TerminationConditionDefinition &terminationConditionDefinition);

 private:
  TerminationFunction instantiateTerminationCondition(TerminationState state, uint32_t playerId, int32_t reward, int32_t opposingReward, YAML::Node& conditionsNode);

  TerminationFunction resolveConditionArguments(const std::function<bool(int32_t, int32_t)> conditionFunction, YAML::Node &conditionArgumentsNode) const override;
  TerminationFunction resolveAND(const std::vector<TerminationFunction>& conditionList) const override;
  TerminationFunction resolveOR(const std::vector<TerminationFunction>& conditionList) const override;

  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> resolveVariables(CommandArguments &commandArguments) const;
  std::vector<ResolvedTerminationCondition> resolvedTerminationConditions_;

  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> availableVariables_;

  const std::shared_ptr<Grid> grid_;
  const std::vector<std::shared_ptr<Player>> players_;
};
}  // namespace griddly