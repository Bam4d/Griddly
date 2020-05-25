#pragma once

#include <vulkan/vulkan.h>
namespace vk {
struct VulkanConfiguration {
  const char* applicationName = "Griddly";
  uint32_t applicationVersion = VK_MAKE_VERSION(0, 0, 0);
  const char* engineName = "GriddlyEngine";
  uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);
  uint32_t apiVersion = VK_API_VERSION_1_0;
};
}