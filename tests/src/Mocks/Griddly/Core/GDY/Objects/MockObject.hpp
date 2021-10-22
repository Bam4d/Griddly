#pragma once

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockObject : public Object {
 public:
  MockObject()
      : Object("mockObject", 'o', 0, 0, {}, nullptr) {
  }

  MOCK_METHOD(void, init, (glm::ivec2 location, std::shared_ptr<Grid> grid_), ());

  MOCK_METHOD(uint32_t, getZIdx, (), (const));
  MOCK_METHOD(glm::ivec2, getLocation, (), (const));
  MOCK_METHOD(std::string, getObjectName, (), (const));
  MOCK_METHOD(char, getMapCharacter, (), (const));
  MOCK_METHOD(std::string, getObjectRenderTileName, (), (const));
  MOCK_METHOD(uint32_t, getPlayerId, (), (const));
  MOCK_METHOD(std::string, getDescription, (), (const));
  MOCK_METHOD(DiscreteOrientation, getObjectOrientation, (), (const));

  MOCK_METHOD(bool, isPlayerAvatar, (), (const));

  MOCK_METHOD(std::vector<std::shared_ptr<Action>>, getInitialActions, (std::shared_ptr<Action> originatingAction), ());

  MOCK_METHOD(bool, isValidAction, (std::shared_ptr<Action> action), (const));

  MOCK_METHOD(BehaviourResult, onActionSrc, (std::string destinationObjectName, std::shared_ptr<Action> action), (override));
  MOCK_METHOD(BehaviourResult, onActionDst, (std::shared_ptr<Action> action), (override));

  MOCK_METHOD(std::unordered_set<std::string>, getAvailableActionNames, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, std::shared_ptr<int32_t>>), getAvailableVariables, (), (const));

  MOCK_METHOD(void, addActionSrcBehaviour, (std::string action, std::string destinationObjectName, std::string commandName, (BehaviourCommandArguments commandArguments), (CommandList conditionalCommands)), (override));
  MOCK_METHOD(void, addActionDstBehaviour, (std::string action, std::string sourceObjectName, std::string commandName, (BehaviourCommandArguments commandArguments), (CommandList conditionalCommands)), (override));
};
}  // namespace griddly