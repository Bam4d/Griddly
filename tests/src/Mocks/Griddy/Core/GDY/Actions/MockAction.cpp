#include "Griddy/Core/GDY/Actions/Action.hpp"
#include "gmock/gmock.h"

namespace griddy {

class MockAction : public Action {
 public:
  MockAction()
      : Action("mockAction", {0, 0}, Direction::NONE) {}

  MockAction(std::string actionName, GridLocation sourceLocation, Direction direction)
      : Action(actionName, sourceLocation, direction) {}

  MOCK_METHOD(GridLocation, getSourceLocation, (), (const));
  MOCK_METHOD(std::string, getActionName, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(GridLocation, getDestinationLocation, (), (const));
  MOCK_METHOD(Direction, getDirection, (), (const));
};
}  // namespace griddy