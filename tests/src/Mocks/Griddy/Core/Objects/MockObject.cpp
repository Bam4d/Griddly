#include "Griddy/Core/GDY/Objects/Object.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockObject : public Object {
 public:
  MOCK_METHOD(uint, getPlayerId, (), (const));
  MOCK_METHOD(GridLocation, getLocation, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(void, setLocation, (GridLocation location), ());
//   MOCK_METHOD(ObjectType, getObjectType, (), (const));


//   MOCK_METHOD(bool, canPerformAction,
//               (std::shared_ptr<Action> action),
//               (override));
  
  
//   MOCK_METHOD(bool, onActionPerformed,
//               (std::shared_ptr<Object> sourceObject,
//                std::shared_ptr<Action> action),
//               (override));

//   MOCK_METHOD(int, onPerformAction,
//               (std::shared_ptr<Object> destinationObject,
//                std::shared_ptr<Action> action),
//               (override));
};
}  // namespace griddy