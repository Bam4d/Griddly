#include "VulkanInstance.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanUtil.hpp"

namespace vk {

    VulkanInstance::VulkanInstance(VulkanConfiguration& config) {

        auto applicationInfo = initializers::applicationInfo(config);
        auto instanceCreateInfo = initializers::instanceCreateInfo(applicationInfo, layers_, extensions_);

        vk_check(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));
    }

    VkInstance& VulkanInstance::getInstance() {
        return instance_;
    }
    

    VulkanInstance::~VulkanInstance() {
        vkDestroyInstance(instance_, NULL);
    }

}