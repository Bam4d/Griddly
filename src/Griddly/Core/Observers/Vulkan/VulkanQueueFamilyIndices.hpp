#pragma once

#include <vulkan/vulkan.h>

namespace vk {
struct VulkanQueueFamilyIndices {
  uint32_t graphicsIndices = UINT32_MAX;
  uint32_t computeIndices = UINT32_MAX;
};
}  // namespace vk