#pragma once
#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddly {

class VulkanGridObserver : public VulkanObserver {
 public:
  VulkanGridObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig);
  ~VulkanGridObserver();

  virtual void renderLocation(vk::VulkanRenderContext& ctx, GridLocation location, float scale, float tileOffset) const = 0;

  std::shared_ptr<uint8_t> update(int playerId) const override;
  std::shared_ptr<uint8_t> reset() const override;

 protected:
  virtual void render(vk::VulkanRenderContext& ctx) const;
};

}  // namespace griddly