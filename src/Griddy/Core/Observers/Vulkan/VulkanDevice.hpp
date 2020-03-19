#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>
#include <cassert>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

struct VulkanRenderContext {
  VkCommandBuffer commandBuffer;
};

struct Vertex;

class VulkanDevice {
 public:
  VulkanDevice(std::unique_ptr<vk::VulkanInstance> vulkanInstance, uint32_t width, uint32_t height);
  ~VulkanDevice();

  void initDevice(bool useGpu);

  // Actual rendering commands
  VulkanRenderContext beginRender();
  void drawSquare(VulkanRenderContext& renderContext, glm::vec3 position);
  void drawTriangle(VulkanRenderContext& renderContext, glm::vec3 position);
  std::unique_ptr<uint8_t[]> endRender(VulkanRenderContext& renderContext);

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

  FrameBufferAttachment createHeadlessRenderSurface(VkPhysicalDevice& physicalDevice);
  void createRenderPass();
  void createGraphicsPipeline();

  void allocateHostImageData(VkPhysicalDevice& physicalDevice);
  std::unique_ptr<uint8_t[]> copySceneToHostImage();

  void submitCommands(VkCommandBuffer cmdBuffer);

  std::unique_ptr<vk::VulkanInstance> vulkanInstance_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue computeQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  // Copy command that gets used when we are pushing data to the rendering device
  VkCommandBuffer copyCmd_ = VK_NULL_HANDLE;

  FrameBufferAttachment colorAttachment_;
  //FrameBufferAttachment depthAttachment_;
  VkFramebuffer frameBuffer_;

  std::unordered_map<std::string, ShapeBuffer> shapeBuffers_;

  VkRenderPass renderPass_;
  bool isRendering_ = false;

  VkDescriptorSetLayout descriptorSetLayout_;
  VkPipelineLayout pipelineLayout_;
  VkPipeline pipeline_;
  VkPipelineCache pipelineCache_;

  // This is where the rendered image data will be 
  VkImage renderedImage_;
  VkDeviceMemory renderedImageMemory_;

  std::vector<VkShaderModule> shaderModules_;

  // Use 16 bit color
  VkFormat colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;

  const uint32_t height_;
  const uint32_t width_;
};
}  // namespace vk