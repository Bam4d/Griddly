#pragma once
#include <vulkan/vulkan.h>

#include <memory>

#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace vk {
class VulkanDevice;
class VulkanInstance;
struct VulkanRenderContext;
struct PersistentSSBOData;
struct FrameSSBOData;
struct ObjectDataSSBO;
struct ShapeBuffer;
struct ObjectSSBOs;
}  // namespace vk

namespace griddly {

struct ResourceConfig {
  std::string imagePath;
  std::string shaderPath;
};

struct ShaderVariableConfig {
  std::vector<std::string> exposedGlobalVariables = {"_steps"};
  std::vector<std::string> exposedObjectVariables = {};
};

class VulkanObserver : public Observer {
 public:
  VulkanObserver(std::shared_ptr<Grid> grid, ResourceConfig observerConfig, ShaderVariableConfig shaderVariableConfig);

  ~VulkanObserver();

  void print(std::shared_ptr<uint8_t> observation) override;

  virtual uint8_t* update() override;
  void init(ObserverConfig observerConfig) override;
  void reset() override;
  void release() override;

 protected:

  virtual glm::mat4 getViewMatrix() = 0;
  virtual vk::PersistentSSBOData updatePersistentShaderBuffers();
  virtual vk::FrameSSBOData updateFrameShaderBuffers() = 0;

  virtual void updateCommandBuffer(std::vector<vk::ObjectDataSSBO> objectData) = 0;

  void resetRenderSurface();

  std::unique_ptr<vk::VulkanDevice> device_;
  const ResourceConfig resourceConfig_;
  const ShaderVariableConfig shaderVariableConfig_;

  uint32_t pixelWidth_;
  uint32_t pixelHeight_;

  bool shouldUpdateCommandBuffer_ = true;

  /**
   * We dont actually want to initialize vulkan on the device unless observations are specifically requested for this environment
   */
  virtual void lazyInit() = 0;

  std::vector<glm::vec4> playerColors_;

 private:
  static std::shared_ptr<vk::VulkanInstance> instance_;
  
};

}  // namespace griddly