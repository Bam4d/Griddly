#include "Griddy/Core/Actions/Action.hpp"
#include "gmock/gmock.h"

namespace griddy {

class MockAction : public Action {
 public:
  MOCK_METHOD(GridLocation, getTargetLocation, (), (const));
  MOCK_METHOD(std::string, getActionTypeName, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
};
}  // namespace griddy