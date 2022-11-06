#pragma once
#include <volk.h>

#include <memory>

#include "../../Grid.hpp"
#include "../Observer.hpp"
#include "../TensorObservationInterface.hpp"
#include "../ObserverConfigInterface.hpp"
#include "VulkanDevice.hpp"

namespace griddly {

struct ResourceConfig {
  std::string gdyPath = "resources/games";
  std::string imagePath = "resources/images";
  std::string shaderPath = "resources/shaders";
};

struct ShaderVariableConfig {
  std::vector<std::string> exposedGlobalVariables = {"_steps"};
  std::vector<std::string> exposedObjectVariables = {};
};

struct VulkanObserverConfig : ObserverConfig {
  ResourceConfig resourceConfig{};
  ShaderVariableConfig shaderVariableConfig{};

  bool highlightPlayers = false;
  std::vector<glm::vec3> playerColors{};
  glm::ivec2 tileSize = {24, 24};
};

class VulkanObserver : public Observer, public TensorObservationInterface, public ObserverConfigInterface<VulkanObserverConfig> {
 public:
  explicit VulkanObserver(std::shared_ptr<Grid> grid);

  ~VulkanObserver() override = default;

  void init(VulkanObserverConfig& config) override;
  void reset() override;
  void release() override;

  std::shared_ptr<ObservationTensor>& update() override;
  std::shared_ptr<ObservationTensor>& getObservationTensor() override;

  virtual const glm::ivec2 getTileSize() const;
  std::vector<int64_t>& getShape() override;
  std::vector<int64_t>& getStrides() override;

 protected:
  virtual glm::mat4 getViewMatrix() = 0;
  virtual vk::PersistentSSBOData updatePersistentShaderBuffers();
  virtual void updateFrameShaderBuffers() = 0;

  virtual void updateCommandBuffer() = 0;

  void resetRenderSurface();

  std::shared_ptr<vk::VulkanDevice> device_;

  uint32_t pixelWidth_;
  uint32_t pixelHeight_;

  bool shouldUpdateCommandBuffer_ = true;

  /**
   * We dont actually want to initialize vulkan on the device unless observations are specifically requested for this environment
   */
  virtual void lazyInit() = 0;

  std::vector<glm::vec4> playerColors_;
  vk::FrameSSBOData frameSSBOData_;

 private:
  static std::shared_ptr<vk::VulkanInstance> instance_;
  VulkanObserverConfig config_;
};

}  // namespace griddly
