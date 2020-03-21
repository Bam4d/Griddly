#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace vk {
class VulkanDevice;
class VulkanInstance;
}  // namespace vk

namespace griddy {

class VulkanObserver : public Observer {
 public:
  VulkanObserver(int tileSize);

  ~VulkanObserver();

  void print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) override;

  void init(int gridWidth, int gridHeight) override;

 protected:
  std::unique_ptr<vk::VulkanDevice> device_;
  const uint32_t tileSize_;

 private:
  std::unique_ptr<vk::VulkanInstance> instance_;
};

}  // namespace griddy