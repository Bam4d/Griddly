#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanDevice.hpp"
#include "SpriteObserver.hpp"

namespace griddly {

struct BlockDefinition {
  float color[3];
  std::string shape;
  float scale;
  float outlineScale = 1.0;
};

class BlockObserver : public SpriteObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions, ShaderVariableConfig shaderVariableConfig);
  ~BlockObserver();


  virtual ObserverType getObserverType() const override;
  std::vector<vk::ObjectSSBOs> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  const std::unordered_map<std::string, BlockDefinition> blockDefinitions_;

  const static std::unordered_map<std::string, SpriteDefinition> blockSpriteDefinitions_;

};

}  // namespace griddly