#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockGDYFactory : public GDYFactory {
 public:

  MockGDYFactory()
  : GDYFactory(nullptr, nullptr) {}

  MOCK_METHOD(std::shared_ptr<TerminationGenerator>, getTerminationGenerator, (), (const));
  MOCK_METHOD(std::shared_ptr<LevelGenerator>, getLevelGenerator, (), (const));
  MOCK_METHOD(std::shared_ptr<ObjectGenerator>, getObjectGenerator, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, SpriteDefinition>), getSpriteObserverDefinitions, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, BlockDefinition>), getBlockObserverDefinitions, (), (const));

  MOCK_METHOD((std::unordered_map<std::string, int32_t>), getGlobalVariableDefinitions, (), (const));

  MOCK_METHOD(std::shared_ptr<TerminationHandler>, createTerminationHandler, (std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players), (const));

  MOCK_METHOD(uint32_t, getTileSize, (), (const));
  MOCK_METHOD(std::string, getName, (), (const));
  MOCK_METHOD(uint32_t, getNumLevels, (), (const));

  MOCK_METHOD(uint32_t, getActionDefinitionCount, (), (const));

  MOCK_METHOD(std::string, getActionName, (uint32_t idx), (const));

  MOCK_METHOD(uint32_t, getPlayerCount, (), (const));
  MOCK_METHOD(PlayerObserverDefinition, getPlayerObserverDefinition, (), (const));
};
}