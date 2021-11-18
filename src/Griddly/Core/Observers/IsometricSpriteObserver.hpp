#pragma once
#include <memory>

#include "SpriteObserver.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

class IsometricSpriteObserver : public SpriteObserver {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions, ShaderVariableConfig shaderVariableConfig);
  ~IsometricSpriteObserver();

  virtual ObserverType getObserverType() const override;

 protected:
  void resetShape() override;

  virtual void addBackgroundTile(std::vector<vk::ObjectDataSSBO>& objectDataSSBOData) override;

  std::vector<vk::ObjectDataSSBO> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  glm::mat4 isoTransform_ = glm::mat4(1.0);
};

}  // namespace griddly