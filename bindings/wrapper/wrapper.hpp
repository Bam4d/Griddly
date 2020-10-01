#pragma once
#include <memory>

#include "../../src/Griddly/Core/Grid.hpp"
#include "../../src/Griddly/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddly/Core/Observers/Observer.hpp"
#include "../../src/Griddly/Core/Observers/SpriteObserver.hpp"
#include "../../src/Griddly/Core/Observers/IsometricSpriteObserver.hpp"
#include "../../src/Griddly/Core/Observers/VectorObserver.hpp"
#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"


namespace griddly {

std::shared_ptr<Observer> createObserver(ObserverType observerType,
                                         std::shared_ptr<Grid> grid,
                                         std::shared_ptr<GDYFactory> gdyFactory,
                                         std::string imagePath,
                                         std::string shaderPath) {

  ResourceConfig resourceConfig = {imagePath, shaderPath};
  switch (observerType) {
    case ObserverType::ISOMETRIC:
      if(gdyFactory->getIsometricSpriteObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Isometric rendering.");
      }

      return std::shared_ptr<IsometricSpriteObserver>(new IsometricSpriteObserver(grid, resourceConfig, gdyFactory->getIsometricSpriteObserverDefinitions()));
      break;
    case ObserverType::SPRITE_2D:
      if(gdyFactory->getSpriteObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Sprite2D rendering.");
      }

      return std::shared_ptr<SpriteObserver>(new SpriteObserver(grid, resourceConfig, gdyFactory->getSpriteObserverDefinitions()));
      break;
    case ObserverType::BLOCK_2D:
      if(gdyFactory->getBlockObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Block2D rendering.");
      }

      return std::shared_ptr<BlockObserver>(new BlockObserver(grid, resourceConfig, gdyFactory->getBlockObserverDefinitions()));
      break;
    case ObserverType::VECTOR:
      return std::shared_ptr<VectorObserver>(new VectorObserver(grid));
      break;
    //case ObserverType::ASCII:
    //  return std::shared_ptr<ASCIIObserver>(new ASCIIObserver(grid));
    //  break;
    case ObserverType::NONE:
      return nullptr;
    default:
      return nullptr;
  }
}
}  // namespace griddly
