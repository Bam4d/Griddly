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
  VulkanObserver(std::shared_ptr<Grid> grid, size_t tileSize);

  ~VulkanObserver();

  void print(std::unique_ptr<uint8_t[]> observation) override;

  void init(size_t gridWidth, size_t gridHeight) override;

  std::vector<size_t> getShape() const override;
  std::vector<size_t> getStrides() const override;

 protected:
  std::unique_ptr<vk::VulkanDevice> device_;
  const size_t tileSize_;

 private:
  std::unique_ptr<vk::VulkanInstance> instance_;
  std::vector<size_t> observationShape_;
  std::vector<size_t> observationStrides_;
};

}  // namespace griddy