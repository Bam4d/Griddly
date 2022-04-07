#pragma once

#include <vector>

#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockTerminationGenerator : public TerminationGenerator {
 public:
  MockTerminationGenerator() : TerminationGenerator(){};

  MOCK_METHOD(void, defineTerminationCondition, (TerminationState state, int32_t reward, int32_t opposingReward, YAML::Node& conditionNode), ());
  MOCK_METHOD(std::shared_ptr<TerminationHandler>, newInstance, (std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players), ());
};
}  // namespace griddly