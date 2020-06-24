#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockAction : public Action {
 public:
  MockAction()
      : Action("mockAction", {0, 0}, 0) {}

  MockAction(std::string actionName, GridLocation sourceLocation, uint32_t actionId)
      : Action(actionName, sourceLocation, actionId) {}

  MOCK_METHOD(uint32_t, getDelay, (), (const));
  MOCK_METHOD(GridLocation, getSourceLocation, (), (const));
  MOCK_METHOD(std::string, getActionName, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(GridLocation, getDestinationLocation, (std::shared_ptr<Object> object), (const));
  MOCK_METHOD(Direction, getDirection, (std::shared_ptr<Object> object), (const));
};
}  // namespace griddly