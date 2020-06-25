#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddly {

class VulkanGridObserver : public VulkanObserver {
 public:
  VulkanGridObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig);
  ~VulkanGridObserver();

  virtual void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, float tileOffset, Direction orientation) const = 0;

  std::shared_ptr<uint8_t> update(int playerId) const override;
  std::shared_ptr<uint8_t> reset() const override;

 protected:
  virtual void render(vk::VulkanRenderContext& ctx) const;

  const static std::vector<glm::vec4> globalObserverPlayerColors_;
};

}  // namespace griddly