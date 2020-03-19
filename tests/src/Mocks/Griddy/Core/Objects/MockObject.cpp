#include "Griddy/Core/Objects/Object.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockObject : public Object {
 public:
  MOCK_METHOD(GridLocation, getLocation, (), ());
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(void, setLocation, (GridLocation location), ());
  MOCK_METHOD(ObjectType, getObjectType, (), (const));
  MOCK_METHOD(bool, onActionPerformed,
              (std::shared_ptr<Object> sourceObject,
               std::shared_ptr<Action> action),
              (override));
};
}  // namespace griddy