#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace vk {

class VulkanInstance;
class VulkanPhysicalDeviceInfo;
class VulkanQueueFamilyIndices;

struct BufferAndMemory {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

struct ShapeBuffer {
  BufferAndMemory vertex;
  BufferAndMemory index;
};

namespace shapes {
struct Shape;
}

struct FrameBufferAttachment {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

struct Vertex;

class VulkanDevice {
 public:
  VulkanDevice(VulkanInstance& instance_);
  ~VulkanDevice();

  void initDevice(bool useGpu, uint32_t height, uint32_t width);

 private:
  std::vector<VkPhysicalDevice> getAvailablePhysicalDevices();
  VulkanPhysicalDeviceInfo getPhysicalDeviceInfo(VkPhysicalDevice& device);
  std::vector<VulkanPhysicalDeviceInfo>::iterator selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices);
  std::vector<VulkanPhysicalDeviceInfo> getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices);
  bool hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices);

  std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice& physicalDevice);

  uint32_t findMemoryTypeIndex(VkPhysicalDevice& physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties);

  std::unordered_map<std::string, ShapeBuffer> createShapeBuffers(VkPhysicalDevice& physicalDevice);
  ShapeBuffer createShapeBuffer(VkPhysicalDevice& physicalDevice, shapes::Shape shape);
  void createBuffer(VkPhysicalDevice& physicalDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data = nullptr);
  BufferAndMemory createVertexBuffers(VkPhysicalDevice& physicalDevice, std::vector<Vertex>& vertices);
  BufferAndMemory createIndexBuffers(VkPhysicalDevice& physicalDevice, std::vector<uint32_t>& vertices);
  void stageBuffersToDevice(VkPhysicalDevice& physicalDevice, VkBuffer& deviceBuffer, void* data, uint32_t bufferSize);

  FrameBufferAttachment createHeadlessRenderSurface(VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height);
  void createRenderPass();
  void createGraphicsPipeline();

  void submitCommands(VkCommandBuffer cmdBuffer);

  const VulkanInstance& vulkanInstance_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue computeQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  // Copy command that gets used when we are pushing data to the rendering device
  VkCommandBuffer copyCmd_ = VK_NULL_HANDLE;

  FrameBufferAttachment colorAttachment_;
  //FrameBufferAttachment depthAttachment_;

  std::unordered_map<std::string, ShapeBuffer> shapeBuffers_;
  
  VkRenderPass renderPass_;

  VkDescriptorSetLayout descriptorSetLayout_;
	VkPipelineLayout pipelineLayout_;
	VkPipeline pipeline_;
  VkPipelineCache pipelineCache_;

  std::vector<VkShaderModule> shaderModules_;

  VkFormat colorFormat_ = VK_FORMAT_R8G8B8A8_UINT;
};
}  // namespace vk