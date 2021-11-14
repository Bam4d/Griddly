#pragma once

#include "Griddly/Core/Grid.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockGrid : public Grid {
 public:
  MockGrid() : Grid() {}

  MOCK_METHOD(void, resetMap, (uint32_t width, uint32_t height), ());
  MOCK_METHOD(void, setGlobalVariables, ((std::unordered_map<std::string, std::unordered_map<uint32_t, int32_t>>)), ());
  MOCK_METHOD(void, resetGlobalVariables, ((std::unordered_map<std::string, GlobalVariableDefinition>)), ());
  MOCK_METHOD((std::unordered_map<uint32_t, int32_t>), update, (), ());

  MOCK_METHOD((const std::unordered_set<glm::ivec2>&), getUpdatedLocations, (uint32_t playerId), (const));
  MOCK_METHOD(void, purgeUpdatedLocations, (uint32_t playerId), ());

  MOCK_METHOD(uint32_t, getWidth, (), (const));
  MOCK_METHOD(uint32_t, getHeight, (), (const));

  MOCK_METHOD(bool, invalidateLocation, (glm::ivec2 location));

  MOCK_METHOD(bool, updateLocation, (std::shared_ptr<Object> object, glm::ivec2 previousLocation, glm::ivec2 newLocation), ());
  MOCK_METHOD((std::unordered_map<uint32_t, int32_t>), performActions, (uint32_t playerId, std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(void, initObject, (std::string, std::vector<std::string>), ());
  MOCK_METHOD(void, addObject, (glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions, std::shared_ptr<Action> originatingAction), ());
  MOCK_METHOD(void, addPlayerDefaultObject, (std::shared_ptr<Object> object));
  MOCK_METHOD(std::shared_ptr<Object>, getPlayerDefaultObject, (uint32_t playerId), (const));
  MOCK_METHOD(bool, removeObject, (std::shared_ptr<Object> object), ());

  MOCK_METHOD((std::unordered_map<uint32_t, std::shared_ptr<int32_t>>), getObjectCounter, (std::string), ());

  MOCK_METHOD((const std::unordered_map<std::string, uint32_t>&), getObjectIds, (), (const));
  MOCK_METHOD((const std::unordered_map<std::string, uint32_t>&), getObjectVariableIds, (), (const));
  MOCK_METHOD((const std::vector<std::string>), getObjectVariableNames, (), (const));
  MOCK_METHOD((const std::vector<std::string>), getObjectNames, (), (const));

  MOCK_METHOD((const std::unordered_map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>&), getGlobalVariables, (), (const));

  MOCK_METHOD((const std::unordered_set<std::shared_ptr<Object>>&), getObjects, (), ());
  MOCK_METHOD(std::shared_ptr<Object>, getObject, (glm::ivec2 location), (const));
  MOCK_METHOD((const TileObjects&), getObjectsAt, (glm::ivec2 location), (const));

  MOCK_METHOD((std::unordered_map<uint32_t, std::shared_ptr<Object>>), getPlayerAvatarObjects, (), (const));
  MOCK_METHOD(void, setPlayerCount, (int32_t), ());
  MOCK_METHOD(uint32_t, getPlayerCount, (), (const));

  MOCK_METHOD(void, addCollisionDetector, (std::vector<std::string> objectNames, std::string actionName, std::shared_ptr<CollisionDetector> collisionDetector), ());

  MOCK_METHOD(std::shared_ptr<int32_t>, getTickCount, (), (const));
};
}  // namespace griddly