#pragma once
#include <memory>

#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddly/Core/Observers/Observer.hpp"
#include "../../src/Griddly/Core/Observers/SpriteObserver.hpp"
#include "../../src/Griddly/Core/Observers/IsometricSpriteObserver.hpp"
#include "../../src/Griddly/Core/Observers/VectorObserver.hpp"

namespace griddly {

enum class ObserverType { NONE,
                          SPRITE_2D,
                          BLOCK_2D,
                          ISOMETRIC,
                          VECTOR };

std::shared_ptr<Observer> createObserver(ObserverType observerType,
                                         std::shared_ptr<Grid> grid,
                                         std::shared_ptr<GDYFactory> gdyFactory,
                                         std::string imagePath,
                                         std::string shaderPath) {

  VulkanObserverConfig vulkanObserverConfig;
  vulkanObserverConfig.tileSize = gdyFactory->getTileSize();
  vulkanObserverConfig.shaderPath = shaderPath;
  vulkanObserverConfig.imagePath = imagePath;
  switch (observerType) {
    case ObserverType::ISOMETRIC:
      return std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(grid, vulkanObserverConfig, gdyFactory->getIsometricSpriteObserverDefinitions()));
      break;
    case ObserverType::SPRITE_2D:
      return std::shared_ptr<SpriteObserver>(new SpriteObserver(grid, vulkanObserverConfig, gdyFactory->getSpriteObserverDefinitions()));
      break;
    case ObserverType::BLOCK_2D:
      return std::shared_ptr<BlockObserver>(new BlockObserver(grid, vulkanObserverConfig, gdyFactory->getBlockObserverDefinitions()));
      break;
    case ObserverType::VECTOR:
      return std::shared_ptr<VectorObserver>(new VectorObserver(grid));
      break;
    case ObserverType::NONE:
      return nullptr;
    default:
      return nullptr;
  }
}
}  // namespace griddly