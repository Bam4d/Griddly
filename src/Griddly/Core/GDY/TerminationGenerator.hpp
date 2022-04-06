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
  virtual void defineTerminationCondition(TerminationState state, int32_t reward, int32_t opposingReward, YAML::Node& conditionsNode);
  virtual std::shared_ptr<TerminationHandler> newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players);

  virtual ~TerminationGenerator() = default;

 private:
  std::vector<TerminationConditionDefinition> terminationConditionDefinitions_;
};
}  // namespace griddly