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
  float outlineScale = 1.0;
};

struct BlockConfig {
  glm::vec3 color;
  vk::ShapeBuffer shapeBuffer;
  float scale;
  float outlineScale;
};

class BlockObserver : public VulkanGridObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions);
  ~BlockObserver();

  virtual ObserverType getObserverType() const override;

 private:
  void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation renderOrientation) const override;
  std::unordered_map<std::string, BlockConfig> blockConfigs_;
  const std::unordered_map<std::string, BlockDefinition> blockDefinitions_;

  void lazyInit() override;
};

}  // namespace griddly