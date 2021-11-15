#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Grid.hpp"

#define TerminationFunction std::function<TerminationResult()>

namespace griddly {

class Player;

enum class TerminationState {
  WIN,
  LOSE,
  NONE  // There does not have to be a winner or loser, just terminate
};

struct TerminationResult {
  bool terminated = false;
  std::unordered_map<uint32_t, int32_t> rewards;
  std::unordered_map<uint32_t, TerminationState> playerStates;
};

struct TerminationConditionDefinition {
  TerminationState state = TerminationState::NONE;
  std::string commandName;
  int32_t reward;
  int32_t opposingReward;
  std::vector<std::string> commandArguments;
};

class TerminationHandler {
 public:
  TerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players);
  virtual ~TerminationHandler() = default;
  virtual TerminationResult isTerminated();

  virtual void addTerminationCondition(TerminationConditionDefinition terminationConditionDefinition);

 private:
  TerminationFunction instantiateTerminationCondition(TerminationState state, std::string commandName, uint32_t playerId, int32_t reward, int32_t opposingReward, std::vector<std::shared_ptr<int32_t>> variablePointers);
  void resolveTerminationConditions(TerminationState state, std::string commandName, int32_t reward, int32_t opposingReward, std::vector<std::string> terminationVariables);

  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> findVariables(std::vector<std::string> variables);
  std::vector<TerminationFunction> terminationFunctions_;

  std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> availableVariables_;

  const std::shared_ptr<Grid> grid_;
  const std::vector<std::shared_ptr<Player>> players_;
};
}  // namespace griddly