#include "TerminationGenerator.hpp"
#include "TerminationHandler.hpp"
#include "../Players/Player.hpp"

#include <spdlog/spdlog.h>

namespace griddly {

void TerminationGenerator::defineTerminationCondition(TerminationState state, std::string commandName, std::vector<std::string> commandParameters) {

  spdlog::debug("Adding termination condition definition {0}", commandName);
  TerminationConditionDefinition tcd;
  tcd.commandName = commandName;
  tcd.commandParameters = commandParameters;
  tcd.state = state;
  terminationConditionDefinitions_.push_back(tcd);
}

std::shared_ptr<TerminationHandler> TerminationGenerator::newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) {

  auto terminationHandler = std::shared_ptr<TerminationHandler>(new TerminationHandler(grid, players));

  for(auto terminationConditionDefinition: terminationConditionDefinitions_) {
    terminationHandler->addTerminationCondition(terminationConditionDefinition);
  }

  return terminationHandler;
}



}  // namespace griddly