#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanConfiguration.hpp"

namespace vk {
class VulkanInstance {
 public:
  ~VulkanInstance();
  VulkanInstance(VulkanConfiguration& config);

  [[nodiscard]] VkInstance getInstance() const;

 private:
  VkInstance instance_;
  std::vector<const char*> layers_;
  std::vector<const char*> extensions_;
};
}  // namespace vk