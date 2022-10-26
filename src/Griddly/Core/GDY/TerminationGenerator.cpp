#include "TerminationGenerator.hpp"

#include <spdlog/spdlog.h>

#include "../Players/Player.hpp"
#include "TerminationHandler.hpp"

namespace griddly {

void TerminationGenerator::defineTerminationCondition(TerminationState state, int32_t reward, int32_t opposingReward, YAML::Node& conditionsNode) {
  TerminationConditionDefinition tcd;
  tcd.conditionsNode = conditionsNode;
  tcd.state = state;
  tcd.reward = reward;
  tcd.opposingReward = opposingReward;
  terminationConditionDefinitions_.push_back(tcd);
}

std::shared_ptr<TerminationHandler> TerminationGenerator::newInstance(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) {
  auto terminationHandler = std::make_shared<TerminationHandler>(TerminationHandler(grid, players));

  for (auto terminationConditionDefinition : terminationConditionDefinitions_) {
    terminationHandler->addTerminationCondition(terminationConditionDefinition);
  }

  return terminationHandler;
}

}  // namespace griddly