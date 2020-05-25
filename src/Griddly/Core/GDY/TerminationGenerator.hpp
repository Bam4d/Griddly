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
  virtual void defineTerminationCondition(TerminationState state, std::string commandName, std::vector<std::string> commandParameters);
  virtual std::shared_ptr<TerminationHandler> newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players);

 private:
  std::vector<TerminationConditionDefinition> terminationConditionDefinitions_;
};
}  // namespace griddly