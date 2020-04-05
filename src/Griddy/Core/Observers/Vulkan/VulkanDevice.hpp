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
  uint32_t indices;
  BufferAndMemory vertex;
  BufferAndMemory index;
};

struct ImageBuffer {
  VkImage image;
  VkDeviceMemory memory;
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
  VulkanDevice(std::unique_ptr<vk::VulkanInstance> vulkanInstance, uint32_t width, uint32_t height, uint32_t tileSize);
  ~VulkanDevice();

  void initDevice(bool useGpu);

  // Actual rendering commands
  VulkanRenderContext beginRender();

  ShapeBuffer getShapeBuffer(std::string shapeBufferName);
  void drawShape(VulkanRenderContext& renderContext, ShapeBuffer shapeBuffer, glm::mat4 model, glm::vec3 color);
  std::unique_ptr<uint8_t[]> endRender(VulkanRenderContext& renderContext);

 private:
  std::vector<VkPhysicalDevice> getAvailablePhysicalDevices();
  VulkanPhysicalDeviceInfo getPhysicalDeviceInfo(VkPhysicalDevice& device);
  std::vector<VulkanPhysicalDeviceInfo>::iterator selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices);
  std::vector<VulkanPhysicalDeviceInfo> getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices);
  bool hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices);

  std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice& physicalDevice);

  uint32_t findMemoryTypeIndex(VkPhysicalDevice& physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties);

  ImageBuffer createSprite(std::unique_ptr<uint8_t[]> imageData, uint32_t imageSize);

  ImageBuffer createImage(VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height, VkFormat colorFormat, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

  std::unordered_map<std::string, ShapeBuffer> createShapeBuffers(VkPhysicalDevice& physicalDevice);
  ShapeBuffer createShapeBuffer(VkPhysicalDevice& physicalDevice, shapes::Shape shape);
  void createBuffer(VkPhysicalDevice& physicalDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data = nullptr);
  BufferAndMemory createVertexBuffers(VkPhysicalDevice& physicalDevice, std::vector<Vertex>& vertices);
  BufferAndMemory createIndexBuffers(VkPhysicalDevice& physicalDevice, std::vector<uint32_t>& vertices);
  void stageBuffersToDevice(VkPhysicalDevice& physicalDevice, VkBuffer& deviceBuffer, void* data, uint32_t bufferSize);

  FrameBufferAttachment createDepthAttachment(VkPhysicalDevice& physicalDevice);
  FrameBufferAttachment createColorAttachment(VkPhysicalDevice& physicalDevice);
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
  FrameBufferAttachment depthAttachment_;
  VkFramebuffer frameBuffer_;

  std::unordered_map<std::string, ShapeBuffer> shapeBuffers_;

  std::unordered_map<std::string, SpriteBuffer> shapeBuffers_;

  VkRenderPass renderPass_;
  bool isRendering_ = false;

  VkDescriptorSetLayout descriptorSetLayout_;
  VkPipelineLayout pipelineLayout_;
  VkPipeline pipeline_;
  VkPipelineCache pipelineCache_;

  // This is where the rendered image data will be 
  VkImage renderedImage_;
  VkDeviceMemory renderedImageMemory_;
  uint8_t* imageRGBA_;

  std::vector<VkShaderModule> shaderModules_;

  // Use 8 bit color
  VkFormat colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat_;

  const uint height_;
  const uint width_;
  const uint tileSize_;
  const glm::mat4 ortho_;
};
}  // namespace vk