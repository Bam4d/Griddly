#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Grid.hpp"

#define TerminationFunction std::function<TerminationResult()>

namespace griddy {

struct TerminationResult {
  bool terminated = false;
  int32_t winningPlayer = 0;
};

class TerminationHandler {
 public:
  TerminationHandler(std::shared_ptr<Grid> grid);
  ~TerminationHandler();
  virtual TerminationResult isTerminated();

  virtual void addTerminationCondition(std::pair<std::string, std::vector<std::string>> terminationConditionDefinition);

 private:
  TerminationFunction instantiateTerminationCondition(std::string commandName, uint32_t playerId, std::vector<std::shared_ptr<int32_t>> parameterPointers);
  void resolveTerminationConditions(std::string commandName, std::vector<std::string> terminationParameters);

  std::vector<std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> findParameters(std::vector<std::string> parameters);
  std::vector<TerminationFunction> terminationFunctions_;

  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters_;

  const std::shared_ptr<Grid> grid_;
};
}  // namespace griddy