#include "Griddy/Core/GDY/Objects/Object.hpp"
#include "gmock/gmock.h"

namespace griddy {

class MockObject : public Object {
 public:

  MockObject()
      : Object("mockObject", 0, {}) {
  }

  MockObject(std::string objectName, uint32_t id, std::unordered_map<std::string, std::shared_ptr<int32_t>> availableParameters)
      : Object(objectName, id, availableParameters) {
  }

  MOCK_METHOD(void, init, (uint32_t playerId, GridLocation location, std::shared_ptr<Grid> grid_), ());

  MOCK_METHOD(GridLocation, getLocation, (), (const));
  MOCK_METHOD(std::string, getObjectName, (), (const));
  MOCK_METHOD(uint32_t, getObjectId, (), (const));
  MOCK_METHOD(uint32_t, getPlayerId, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));

  MOCK_METHOD(bool, canPerformAction, (std::string actionName), (const));

  MOCK_METHOD(BehaviourResult, onActionSrc, (std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action), (override));
  MOCK_METHOD(BehaviourResult, onActionDst, (std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action), (override));

  MOCK_METHOD(void, addActionSrcBehaviour, (std::string action, std::string destinationObjectName, std::string commandName, std::vector<std::string> commandParameters, (std::unordered_map<std::string, std::vector<std::string>> nestedCommands)), (override));
  MOCK_METHOD(void, addActionDstBehaviour, (std::string action, std::string sourceObjectName, std::string commandName, std::vector<std::string> commandParameters, (std::unordered_map<std::string, std::vector<std::string>> nestedCommands)), (override));

};
}  // namespace griddy