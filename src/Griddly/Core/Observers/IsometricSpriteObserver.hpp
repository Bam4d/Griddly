#pragma once
#include <memory>

#include "VulkanGridObserver.hpp"
#include "SpriteObserver.hpp"


namespace griddly {

class IsometricSpriteObserver : public SpriteObserver {
 public:
  IsometricSpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~IsometricSpriteObserver();

  void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation orientation) const override;

protected:
  void render(vk::VulkanRenderContext& ctx) const override;
};

}  // namespace griddly