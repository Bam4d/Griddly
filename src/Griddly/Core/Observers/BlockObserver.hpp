#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanDevice.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

struct BlockDefinition {
  float color[3];
  std::string shape;
  float scale;
};

struct BlockConfig {
  glm::vec3 color;
  vk::ShapeBuffer shapeBuffer;
  float scale;
};

class BlockObserver : public VulkanGridObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions);
  ~BlockObserver();

  void init(ObserverConfig observerConfig) override;

  void renderLocation(vk::VulkanRenderContext& ctx, GridLocation objectLocation, GridLocation outputLocation, float scale, float tileOffset, Direction orientation) const override;

 private:
  std::unordered_map<std::string, BlockConfig> blockConfigs_;
  const std::unordered_map<std::string, BlockDefinition> blockDefinitions_;
};

}  // namespace griddly