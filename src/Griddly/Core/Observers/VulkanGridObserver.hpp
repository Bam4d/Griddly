#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddly {

class VulkanGridObserver : public VulkanObserver {
 public:
  VulkanGridObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig);
  ~VulkanGridObserver();


 protected:
  virtual void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation renderOrientation) const = 0;
  virtual void render(vk::VulkanRenderContext& ctx) const;
  virtual void resetRenderSurface() override;
  virtual std::vector<VkRect2D> calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const;
  const static std::vector<glm::vec4> globalObserverPlayerColors_;
};

}  // namespace griddly