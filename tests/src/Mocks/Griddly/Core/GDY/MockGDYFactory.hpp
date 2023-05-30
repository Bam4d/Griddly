#pragma once

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockGDYFactory : public GDYFactory {
 public:
  MockGDYFactory()
      : GDYFactory(nullptr, nullptr, {}) {}

  MOCK_METHOD(std::shared_ptr<TerminationGenerator>, getTerminationGenerator, (), (const));
  MOCK_METHOD(std::shared_ptr<LevelGenerator>, getLevelGenerator, (uint32_t), (const));
  MOCK_METHOD(std::shared_ptr<ObjectGenerator>, getObjectGenerator, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, SpriteDefinition>), getSpriteObserverDefinitions, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, BlockDefinition>), getBlockObserverDefinitions, (), (const));

  MOCK_METHOD((std::map<std::string, GlobalVariableDefinition>&), getGlobalVariableDefinitions, (), (const));

  MOCK_METHOD(std::shared_ptr<TerminationHandler>, createTerminationHandler, (std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players), (const));
  MOCK_METHOD(std::shared_ptr<Observer>, createObserver, (std::shared_ptr<Grid> grid, std::string observerName, uint32_t playerCount, uint32_t playerId), ());

  MOCK_METHOD(glm::ivec2, getTileSize, (), (const));
  MOCK_METHOD(std::string, getName, (), (const));
  MOCK_METHOD(uint32_t, getNumLevels, (), (const));

  MOCK_METHOD(uint32_t, getActionDefinitionCount, (), (const));
  MOCK_METHOD((std::unordered_map<std::string, ActionInputsDefinition>), getActionInputsDefinitions, (), (const));

  MOCK_METHOD(std::string, getActionName, (uint32_t idx), (const));

  MOCK_METHOD(uint32_t, getPlayerCount, (), (const));
  MOCK_METHOD(DefaultObserverConfig, getPlayerObserverDefinition, (), (const));
};
}  // namespace griddly