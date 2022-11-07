#include "VulkanInstance.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanInstance::VulkanInstance(VulkanConfiguration& config) {

  vk_check(volkInitialize());
  
  auto applicationInfo = initializers::applicationInfo(config);
  auto instanceCreateInfo = initializers::instanceCreateInfo(applicationInfo, layers_, extensions_);

#ifndef NDEBUG

  const char* enabledLayerNames[] = {
      "VK_LAYER_KHRONOS_validation",
  };
  int layerCount = 1;

  const char* enabledExtensionNames[] = {
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
      VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  int extensionCount = 3;
#else
  const char* enabledLayerNames[] = {""};
  int layerCount = 0;

  const char* enabledExtensionNames[] = {
      VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
      VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  int extensionCount = 2;

#endif
  // Check if layers are available
  uint32_t instanceLayerCount;
  vk_check(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vk_check(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data()));

  uint32_t instanceExtensionCount;
  vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
  std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
  vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()));

  bool layersAvailable = true;
  if (layerCount > 0) {
    for (auto layerName : enabledLayerNames) {
      bool layerAvailable = false;
      for (auto instanceLayer : instanceLayers) {
        if (strcmp(instanceLayer.layerName, layerName) == 0) {
          layerAvailable = true;
          break;
        }
      }
      if (!layerAvailable) {
        spdlog::warn("Required vulkan layer unavailable: {0}", layerName);
        layersAvailable = false;
        break;
      }
    }
  }

  bool extensionsAvailable = true;
  if (extensionCount > 0) {
    for (auto extensionName : enabledExtensionNames) {
      bool extensionAvailable = false;
      for (auto instanceExtension : instanceExtensions) {
        if (strcmp(instanceExtension.extensionName, extensionName) == 0) {
          extensionAvailable = true;
          break;
        }
      }
      if (!extensionAvailable) {
        spdlog::warn("Required vulkan extension unavailable: {0}", extensionName);
        extensionsAvailable = false;
        break;
      }
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

  vk_check(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));

  volkLoadInstance(instance_);
}

VkInstance VulkanInstance::getInstance() const {
  return instance_;
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(instance_, nullptr);
}

}  // namespace vk