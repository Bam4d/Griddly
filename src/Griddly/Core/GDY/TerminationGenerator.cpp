#include "TerminationGenerator.hpp"

#include <spdlog/spdlog.h>

#include "../Players/Player.hpp"
#include "TerminationHandler.hpp"

namespace griddly {

void TerminationGenerator::defineTerminationCondition(TerminationState state, std::string commandName, int32_t reward, int32_t opposingReward, std::vector<std::string> commandArguments) {
  spdlog::debug("Adding termination condition definition {0} [{1}, {2}]", commandName, commandArguments[0], commandArguments[1]);
  TerminationConditionDefinition tcd;
  tcd.commandName = commandName;
  tcd.commandArguments = commandArguments;
  tcd.state = state;
  tcd.reward = reward;
  tcd.opposingReward = opposingReward;
  terminationConditionDefinitions_.push_back(tcd);
}

std::shared_ptr<TerminationHandler> TerminationGenerator::newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) {
  auto terminationHandler = std::shared_ptr<TerminationHandler>(new TerminationHandler(grid, players));

  for (auto terminationConditionDefinition : terminationConditionDefinitions_) {
    terminationHandler->addTerminationCondition(terminationConditionDefinition);
  }

  return terminationHandler;
}

}  // namespace griddly