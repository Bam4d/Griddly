#include "Griddy/Core/Actions/Action.hpp"
#include "gmock/gmock.h"

namespace griddy {

class MockAction : public Action {
 public:
  MockAction(ActionType actionType) : Action({0,0}, "MockAction", actionType) {}
  MockAction(int x, int y) : Action({x,y}, "MockAction", ActionType::MOVE) {}

  MOCK_METHOD(GridLocation, getTargetLocation, (), (const));
  MOCK_METHOD(std::string, getActionTypeName, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(GridLocation, getDestinationLocation, (), (const));
};
}  // namespace griddy