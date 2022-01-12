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
  virtual vk::FrameSSBOData updateFrameShaderBuffers() override;

  virtual std::vector<vk::ObjectSSBOs> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalObserverMatrix, DiscreteOrientation globalOrientation) = 0;

  void resetShape() override;

  std::unordered_map<std::shared_ptr<Object>, vk::ObjectSSBOs> objectSSBODataCache_;

 private:
  uint32_t commandBufferObjectsCount_ = 0;
};

}  // namespace griddly