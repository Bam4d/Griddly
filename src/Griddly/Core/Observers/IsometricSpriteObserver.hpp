#pragma once
#include <memory>

#include "SpriteObserver.hpp"
#include "VulkanGridObserver.hpp"
#include "ObserverConfigInterface.hpp"

namespace griddly {

struct IsometricSpriteObserverConfig : public GridObserverConfig {
  uint32_t isoTileDepth = 0;
  uint32_t isoTileHeight = 0;
};

class IsometricSpriteObserver : public SpriteObserver, public ObserverConfigInterface<IsometricSpriteObserverConfig> {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~IsometricSpriteObserver() override = default;

  ObserverType getObserverType() const override;

  void init(IsometricSpriteObserverConfig& config) override {
    SpriteObserver::init(config);
    config_ = config;
  }

 protected:
  glm::mat4 getViewMatrix() override;
  glm::mat4 getGlobalModelMatrix() override;
  void resetShape() override;

  void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  glm::mat4 isoTransform_ = glm::mat4(1.0);
  float isoHeightRatio_ = 1.0;

  IsometricSpriteObserverConfig config_{};


};

}  // namespace griddly