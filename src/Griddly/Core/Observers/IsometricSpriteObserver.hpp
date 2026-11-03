#pragma once
#include <memory>

#include "SDLGridObserver.hpp"
#include "SpriteObserver.hpp"

namespace griddly {

struct IsometricSpriteObserverConfig : public SpriteObserverConfig {
  uint32_t isoTileDepth = 0;
  uint32_t isoTileHeight = 0;
};

class IsometricSpriteObserver : public SpriteObserver {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, IsometricSpriteObserverConfig& config);
  ~IsometricSpriteObserver() override = default;

  ObserverType getObserverType() const override;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

 protected:
  void resetShape() override;
  glm::vec2 getTilePixelCenter(glm::ivec2 location) const override;
  void updateObjectRenderItems(PartialObservableGrid& partiallyObservableGrid) override;

 private:
  float isoHeightRatio_ = 1.0;

  IsometricSpriteObserverConfig config_{};
};

}  // namespace griddly
