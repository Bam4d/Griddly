#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#define TerminationFunction std::function<TerminationResult>

namespace griddy {

enum class TerminationBehaviourType {
  WIN,
  LOSE
};

struct TerminationConditionDefinition {
  TerminationBehaviourType behaviourType;
  std::unordered_map<std::string, std::vector<std::string>> conditionalCommands;
};

struct TerminationResult {
  bool terminated;
  int32_t winningPlayer = 0;
};

class TerminationHandler {
 public:
  virtual void registerObject(std::string objectName);
  virtual void defineTerminationCondition(TerminationConditionDefinition terminationConditionDefintion);

  virtual TerminationResult isTerminated();

 private:
  std::vector<std::shared_ptr<int32_t>> findParameters(std::vector<std::string> parameters);
  std::vector<TerminationFunction> terminationFunctions_;

  std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters_;
};
}  // namespace griddy