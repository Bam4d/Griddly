#include "Griddly/Core/GDY/TerminationHandler.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockTerminationHandler : public TerminationHandler {
 public:
  MockTerminationHandler(std::shared_ptr<Grid> grid) : TerminationHandler(grid, {}) {
  }

  MOCK_METHOD(TerminationResult, isTerminated, (), ());

  MOCK_METHOD(void, addTerminationCondition, (TerminationConditionDefinition terminationConditionDefinition), ());
};
}  // namespace griddly