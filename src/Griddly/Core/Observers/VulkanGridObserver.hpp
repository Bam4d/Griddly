#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanObserver.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

class VulkanGridObserver : public VulkanObserver {
 public:
  VulkanGridObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, ShaderVariableConfig shaderVariableConfig);
  ~VulkanGridObserver();

 protected:
  virtual glm::mat4 getViewMatrix() override;
  virtual std::vector<int32_t> getExposedVariableValues(std::shared_ptr<Object> object);
  virtual PartialObservableGrid getObservableGrid();
  virtual glm::mat4 getGlobalModelMatrix();
  virtual void updateFrameShaderBuffers() override;

  virtual void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) = 0;

  void resetShape() override;

 private:
  uint32_t commandBufferObjectsCount_ = 0;
};

}  // namespace griddly