#include "Griddly/Core/Grid.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockGrid : public Grid {
 public:
  MockGrid() : Grid() {}

  MOCK_METHOD(void, resetMap, (uint32_t width, uint32_t height), ());
  MOCK_METHOD(void, resetGlobalVariables, ((std::unordered_map<std::string, int32_t>)), ());
  MOCK_METHOD((std::unordered_map<uint32_t, int32_t>), update, (), ());

  MOCK_METHOD((std::unordered_set<glm::ivec2>), getUpdatedLocations, (), (const));

  MOCK_METHOD(uint32_t, getWidth, (), (const));
  MOCK_METHOD(uint32_t, getHeight, (), (const));

  MOCK_METHOD(bool, updateLocation, (std::shared_ptr<Object> object, glm::ivec2 previousLocation, glm::ivec2 newLocation), ());
  MOCK_METHOD(std::vector<int>, performActions, (uint32_t playerId, std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(void, initObject, (std::string), ());
  MOCK_METHOD(void, addObject, (uint32_t playerId, glm::ivec2 location, std::shared_ptr<Object> object, bool applyInitialActions), ());
  MOCK_METHOD(bool, removeObject, (std::shared_ptr<Object> object), ());

  MOCK_METHOD((std::unordered_map<uint32_t, std::shared_ptr<int32_t>>), getObjectCounter, (std::string), ());
  MOCK_METHOD(uint32_t, getUniqueObjectCount, (), (const));

  MOCK_METHOD((std::unordered_map<std::string, std::shared_ptr<int32_t>>), getGlobalVariables, (), (const));

  MOCK_METHOD(std::unordered_set<std::shared_ptr<Object>>&, getObjects, (), ());
  MOCK_METHOD(std::shared_ptr<Object>, getObject, (glm::ivec2 location), (const));
  MOCK_METHOD((TileObjects), getObjectsAt, (glm::ivec2 location), (const));

  MOCK_METHOD((std::unordered_map<uint32_t, std::shared_ptr<Object>>), getPlayerAvatarObjects, (), (const));

  MOCK_METHOD(std::shared_ptr<int32_t>, getTickCount, (), (const));
};
}  // namespace griddly