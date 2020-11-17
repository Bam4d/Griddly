#include "VulkanInstance.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanInstance::VulkanInstance(VulkanConfiguration& config) {
  auto applicationInfo = initializers::applicationInfo(config);
  auto instanceCreateInfo = initializers::instanceCreateInfo(applicationInfo, layers_, extensions_);

#ifndef NDEBUG

  const char* enabledLayerNames[] = {
      "VK_LAYER_KHRONOS_validation",
  };
  int layerCount = 1;

  const char* enabledExtensionNames[] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  int extensionCount = 3;
#else
  const char* enabledLayerNames = NULL;
  int layerCount = 0;

  const char* enabledExtensionNames[] = {
        VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  int extensionCount = 2;

#endif
  // Check if layers are available
  uint32_t instanceLayerCount;
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL);
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

  uint32_t instanceExtensionCount;
  vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, NULL);
  std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
  vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, instanceExtensions.data());

  bool layersAvailable = true;
  for (auto layerName : enabledLayerNames) {
    bool layerAvailable = false;
    for (auto instanceLayer : instanceLayers) {
      if (strcmp(instanceLayer.layerName, layerName) == 0) {
        layerAvailable = true;
        break;
      }
    }
    if (!layerAvailable) {
      layersAvailable = false;
      break;
    }
  }

  bool extensionsAvailable = true;
  for (auto extensionName : enabledExtensionNames) {
    bool extensionAvailable = false;
    for (auto instanceExtension : instanceExtensions) {
      if (strcmp(instanceExtension.extensionName, extensionName) == 0) {
        extensionAvailable = true;
        break;
      }
    }
    if (!extensionAvailable) {
      extensionsAvailable = false;
      break;
    }
  }

  if (layersAvailable && extensionsAvailable) {
    instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames;
    instanceCreateInfo.enabledLayerCount = layerCount;
    
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
  } else {
    spdlog::error("Missing vulkan extensions in driver. Please upgrade your vulkan drivers.");
  }

  vk_check(vkCreateInstance(&instanceCreateInfo, NULL, &instance_));
}

VkInstance VulkanInstance::getInstance() const {
  return instance_;
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(instance_, NULL);
}

}  // namespace vk