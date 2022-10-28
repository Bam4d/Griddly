#include <volk.h>

#include <string>

#include "VulkanQueueFamilyIndices.hpp"

namespace vk {
struct VulkanPhysicalDeviceInfo {
  VkPhysicalDevice physicalDevice;
  std::string deviceName;
  bool isGpu;
  bool isSupported;
  uint8_t gpuIdx;
  uint8_t pciBusId;
  VulkanQueueFamilyIndices queueFamilyIndices;
};
}  // namespace vk