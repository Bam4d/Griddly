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
  BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions, ShaderVariableConfig shaderVariableConfig);
  ~BlockObserver();

  virtual ObserverType getObserverType() const override;
  std::vector<vk::ObjectDataSSBO> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:

  std::unordered_map<std::string, BlockConfig> blockConfigs_;
  const std::unordered_map<std::string, BlockDefinition> blockDefinitions_;

  void lazyInit() override;
};

}  // namespace griddly