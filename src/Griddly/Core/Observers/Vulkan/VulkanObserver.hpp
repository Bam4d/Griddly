#pragma once
#include <volk.h>

#include <memory>

#include "../../Grid.hpp"
#include "../Observer.hpp"
#include "../TensorObservationInterface.hpp"
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

enum class GlobalObserverAvatarMode {
  NONE,
  GRAYSCALE_INVISIBLE,
  DARKEN_INVISIBLE,
};

struct VulkanObserverConfig : ObserverConfig {
  ResourceConfig resourceConfig{};
  ShaderVariableConfig shaderVariableConfig{};

  bool highlightPlayers = false;
  std::vector<glm::vec3> playerColors{};
  glm::ivec2 tileSize = {24, 24};

  GlobalObserverAvatarMode globalObserverAvatarMode = GlobalObserverAvatarMode::NONE;
};

class VulkanObserver : public Observer, public TensorObservationInterface {
 public:
  explicit VulkanObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig& config);

  ~VulkanObserver() override;

  uint8_t& update() override;
  virtual void init(std::vector<std::weak_ptr<Observer>> playerObservers);
  void reset(std::shared_ptr<Object> avatarObject=nullptr) override;
  void release() override;

  virtual const glm::ivec2 getTileSize() const;

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
