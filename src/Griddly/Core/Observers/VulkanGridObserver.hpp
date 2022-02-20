#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanObserver.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "ObserverConfigInterface.hpp"

namespace griddly {


struct VulkanGridObserverConfig : public VulkanObserverConfig {
  bool rotateAvatarImage = true;
};

class VulkanGridObserver : public VulkanObserver, public ObserverConfigInterface<VulkanGridObserverConfig> {
 public:
  explicit VulkanGridObserver(std::shared_ptr<Grid> grid);
  ~VulkanGridObserver() override = default;

  void init(VulkanGridObserverConfig& config) override;

  const VulkanGridObserverConfig& getConfig() const override;

 protected:
  glm::mat4 getViewMatrix() override;
  virtual std::vector<int32_t> getExposedVariableValues(std::shared_ptr<Object> object);
  virtual PartialObservableGrid getObservableGrid();
  virtual glm::mat4 getGlobalModelMatrix();
  void updateFrameShaderBuffers() override;

  virtual void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) = 0;

  void resetShape() override;

 private:
  uint32_t commandBufferObjectsCount_ = 0;
  VulkanGridObserverConfig config_;

};

}  // namespace griddly