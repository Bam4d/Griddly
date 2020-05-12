#include "TerminationGenerator.hpp"
#include "TerminationHandler.hpp"

#include <spdlog/spdlog.h>

namespace griddy {

void TerminationGenerator::defineTerminationCondition(std::string commandName, std::vector<std::string> commandParameters) {

  spdlog::debug("Adding termination condition definition {0}", commandName);
  auto terminationConditionDefinition = std::pair<std::string, std::vector<std::string>>{commandName, commandParameters};
  terminationConditionDefinitions_.push_back(terminationConditionDefinition);
}

std::shared_ptr<TerminationHandler> TerminationGenerator::newInstance(std::shared_ptr<Grid> grid) {

  auto terminationHandler = std::shared_ptr<TerminationHandler>(new TerminationHandler(grid));

  for(auto terminationConditionDefinition: terminationConditionDefinitions_) {
    terminationHandler->addTerminationCondition(terminationConditionDefinition);
  }

  return terminationHandler;
}



}  // namespace griddy