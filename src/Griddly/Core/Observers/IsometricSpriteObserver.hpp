#pragma once
#include <memory>

#include "SpriteObserver.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

class IsometricSpriteObserver : public SpriteObserver {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~IsometricSpriteObserver();

 protected:
  void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation orientation) const override;
  virtual void resetRenderSurface() override;
  glm::vec2 isometricOutputLocation(glm::vec2 outputLocation, glm::vec2 offset) const;
  std::vector<VkRect2D> calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const override;

 private:
  glm::vec2 isoOriginOffset_;
};

}  // namespace griddly