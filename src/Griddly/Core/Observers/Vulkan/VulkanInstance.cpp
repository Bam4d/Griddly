#include "VulkanInstance.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanInstance::VulkanInstance(VulkanConfiguration& config) {

  vk_check(volkInitialize());
  
  auto applicationInfo = initializers::applicationInfo(config);

#ifndef NDEBUG

  const std::vector<const char*> enabledLayerNames = {
      "VK_LAYER_KHRONOS_validation",
  };

  const std::vector<const char*> enabledExtensionNames = {
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
#else
  const std::vector<const char*> enabledLayerNames = {""};

  const std::vector<const char*> enabledExtensionNames = {
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

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
  if (enabledLayerNames.size() > 0) {
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
  if (enabledExtensionNames.size() > 0) {
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

    auto instanceCreateInfo = initializers::instanceCreateInfo(applicationInfo, enabledLayerNames, enabledExtensionNames, VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);

    vk_check(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));
  } else {
    std::string error = "Missing vulkan extensions in driver. Please upgrade your vulkan drivers.";
    spdlog::error(error);
    throw std::runtime_error(error);
  }

  volkLoadInstance(instance_);
}

VkInstance VulkanInstance::getInstance() const {
  return instance_;
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(instance_, nullptr);
}

}  // namespace vk