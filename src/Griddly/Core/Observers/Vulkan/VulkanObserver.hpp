#pragma once
#include <vulkan/vulkan.h>

#include <memory>

#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace vk {
class VulkanDevice;
class VulkanInstance;
struct VulkanRenderContext;
}  // namespace vk

namespace griddly {

struct VulkanObserverConfig {
  glm::ivec2 tileSize;
  std::string imagePath;
  std::string shaderPath;
};

class VulkanObserver : public Observer {
 public:
  VulkanObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig observerConfig);

  ~VulkanObserver();

  void print(std::shared_ptr<uint8_t> observation) override;
  void init(ObserverConfig observerConfig) override;

  virtual std::shared_ptr<uint8_t> update() const override;
  virtual std::shared_ptr<uint8_t> reset() override ;

  void release() override;

 protected:
  virtual void render(vk::VulkanRenderContext& ctx) const = 0;
  
  virtual void resetRenderSurface() = 0;
  virtual std::vector<VkRect2D> calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const = 0;
  
  std::unique_ptr<vk::VulkanDevice> device_;
  VulkanObserverConfig vulkanObserverConfig_;

  uint32_t pixelWidth_;
  uint32_t pixelHeight_;

 private:
  static std::shared_ptr<vk::VulkanInstance> instance_;
};

}  // namespace griddly