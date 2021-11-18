#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddly {

class VulkanGridObserver : public VulkanObserver {
 public:
  VulkanGridObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, ShaderVariableConfig shaderVariableConfig);
  ~VulkanGridObserver();

 protected:
  virtual vk::FrameSSBOData updateFrameShaderBuffers() override;

  virtual std::vector<vk::ObjectDataSSBO> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalObserverMatrix, DiscreteOrientation globalOrientation) = 0;

  void resetShape() override;

 private:
  uint32_t commandBufferObjectsCount_ = 0;
};

}  // namespace griddly