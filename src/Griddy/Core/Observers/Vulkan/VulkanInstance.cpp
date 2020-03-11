#include "VulkanInstance.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanInstance::VulkanInstance(VulkanConfiguration& config) {
  auto applicationInfo = initializers::applicationInfo(config);
  auto instanceCreateInfo = initializers::instanceCreateInfo(applicationInfo, layers_, extensions_);

  const char* validationLayers[] = {"VK_LAYER_LUNARG_standard_validation"};
  int layerCount = 1;

  // Check if layers are available
  uint32_t instanceLayerCount;
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

  bool layersAvailable = true;
  for (auto layerName : validationLayers) {
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

  if (layersAvailable) {
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;
    const char* validationExt = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    instanceCreateInfo.enabledLayerCount = layerCount;
    instanceCreateInfo.enabledExtensionCount = 1;
    instanceCreateInfo.ppEnabledExtensionNames = &validationExt;
  }

  vk_check(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));
}

VkInstance VulkanInstance::getInstance() const {
  return instance_;
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(instance_, NULL);
}

}  // namespace vk