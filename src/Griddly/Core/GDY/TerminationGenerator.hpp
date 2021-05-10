#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "TerminationHandler.hpp"

namespace griddly {

class Grid;
class Player;

class TerminationGenerator {
 public:
  virtual void defineTerminationCondition(TerminationState state, std::string commandName, int32_t reward, int32_t opposingReward, std::vector<std::string> commandArguments);
  virtual std::shared_ptr<TerminationHandler> newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players);

 private:
  std::vector<TerminationConditionDefinition> terminationConditionDefinitions_;
};
}  // namespace griddly