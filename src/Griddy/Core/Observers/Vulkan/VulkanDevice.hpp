#include <vulkan/vulkan.h>
#include <vector>

namespace vk {

class VulkanInstance;
class VulkanPhysicalDeviceInfo;
class VulkanQueueFamilyIndices;

class VulkanDevice {
 public:
  VulkanDevice(VulkanInstance& instance_);
  ~VulkanDevice();

  void initDevice(bool useGpu);

 private:
  std::vector<VkPhysicalDevice> getAvailablePhysicalDevices();
  VulkanPhysicalDeviceInfo getPhysicalDeviceInfo(VkPhysicalDevice& device);
  std::vector<VulkanPhysicalDeviceInfo>::iterator selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices);
  std::vector<VulkanPhysicalDeviceInfo> getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices);
  bool hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices);

  std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice& physicalDevice);

  const VulkanInstance& vulkanInstance_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue computeQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;
};
}  // namespace vk