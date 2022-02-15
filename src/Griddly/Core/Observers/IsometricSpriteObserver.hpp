#pragma once
#include <memory>

#include "SpriteObserver.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

class IsometricSpriteObserver : public SpriteObserver {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions, ShaderVariableConfig shaderVariableConfig);
  ~IsometricSpriteObserver() override;

  ObserverType getObserverType() const override;

 protected:
  glm::mat4 getViewMatrix() override;
  glm::mat4 getGlobalModelMatrix() override;
  void resetShape() override;

  void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  glm::mat4 isoTransform_ = glm::mat4(1.0);
  float isoHeightRatio_;
};

}  // namespace griddly