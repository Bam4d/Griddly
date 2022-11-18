#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanObserver.hpp"

namespace griddly {

struct VulkanGridObserverConfig : public VulkanObserverConfig {
  bool rotateAvatarImage = true;
};

class VulkanGridObserver : public VulkanObserver {
 public:
  explicit VulkanGridObserver(std::shared_ptr<Grid> grid, VulkanGridObserverConfig& config);
  ~VulkanGridObserver() override = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

 protected:
  glm::mat4 getViewMatrix() override;
  virtual std::vector<int32_t> getExposedVariableValues(std::shared_ptr<Object> object);
  virtual glm::mat4 getGlobalModelMatrix();
  void updateFrameShaderBuffers() override;

  virtual void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) = 0;

  void resetShape() override;

 private:
  uint32_t commandBufferObjectsCount_ = 0;
  VulkanGridObserverConfig config_;
};

}  // namespace griddly
