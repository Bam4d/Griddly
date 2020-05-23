#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace vk {
class VulkanDevice;
class VulkanInstance;
struct VulkanRenderContext;
}  // namespace vk

namespace griddle {

class VulkanObserver : public Observer {
 public:
  VulkanObserver(std::shared_ptr<Grid> grid, uint32_t tileSize, std::string resourcePath);

  ~VulkanObserver();

  void print(std::unique_ptr<uint8_t[]> observation) override;

  void init(uint32_t gridWidth, uint32_t gridHeight) override;

  std::vector<uint32_t> getShape() const override;
  std::vector<uint32_t> getStrides() const override;

 protected:
  std::unique_ptr<vk::VulkanDevice> device_;
  const uint32_t tileSize_;
  const std::string resourcePath_;

 private:
  std::unique_ptr<vk::VulkanInstance> instance_;
  std::vector<uint32_t> observationShape_;
  std::vector<uint32_t> observationStrides_;
};

}  // namespace griddle