#pragma once

#include <vector>

#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockTerminationGenerator : public TerminationGenerator {
 public:
  MockTerminationGenerator() : TerminationGenerator(){};

  MOCK_METHOD(void, defineTerminationCondition, (TerminationState state, std::string commandName, int32_t reward, int32_t opposingReward, std::vector<std::string> commandParameters), ());
  MOCK_METHOD(std::shared_ptr<TerminationHandler>, newInstance, (std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players), ());
};
}  // namespace griddly