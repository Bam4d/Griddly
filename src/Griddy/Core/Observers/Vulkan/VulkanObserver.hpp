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
  VulkanObserver(std::shared_ptr<Grid> grid, uint tileSize);

  ~VulkanObserver();

  void print(std::unique_ptr<uint8_t[]> observation) override;

  void init(uint gridWidth, uint gridHeight) override;

  std::vector<uint> getShape() const override;
  std::vector<uint> getStrides() const override;

 protected:
  std::unique_ptr<vk::VulkanDevice> device_;
  const uint tileSize_;

 private:
  std::unique_ptr<vk::VulkanInstance> instance_;
  std::vector<uint> observationShape_;
  std::vector<uint> observationStrides_;
};

}  // namespace griddy