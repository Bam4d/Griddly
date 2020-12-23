#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockObjectGenerator : public ObjectGenerator {
 public:
  MockObjectGenerator() : ObjectGenerator() {}

  MOCK_METHOD(void, defineNewObject, (std::string objectName, uint32_t zIdx, char mapChar, (std::unordered_map<std::string, uint32_t> parameterDefinitions)), ());
  MOCK_METHOD(void, defineActionBehaviour, (std::string objectName, ActionBehaviourDefinition behaviourDefinition), ());
  MOCK_METHOD(void, addInitialAction, (std::string objectName, std::string actionName, uint32_t actionId, uint32_t delay, bool randomize), ());

  MOCK_METHOD((std::unordered_map<std::string, ActionInputsDefinition>), getActionInputDefinitions, (), (const));

  MOCK_METHOD(std::shared_ptr<Object>, newInstance, (std::string objectName, uint32_t playerId, (std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables)), ());

  MOCK_METHOD(std::string&, getObjectNameFromMapChar, (char character), ());
  MOCK_METHOD((std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>>), getObjectDefinitions, (), (const));

  MOCK_METHOD(void, setAvatarObject, (std::string objectName), ());
};
}  // namespace griddly