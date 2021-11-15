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

  glm::vec2 isometricOutputLocation(glm::vec2 outputLocation, glm::vec2 offset) const;

  std::vector<VkRect2D> calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const override;

 private:
  glm::vec2 isoOriginOffset_;
};

}  // namespace griddly