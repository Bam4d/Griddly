#pragma once
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vk {

class VulkanInstance;
class VulkanPhysicalDeviceInfo;
class VulkanQueueFamilyIndices;

enum RenderMode {
  SHAPES,
  SPRITES,
};

struct BufferAndMemory {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

struct ShapeBuffer {
  uint32_t indices;
  BufferAndMemory vertex;
  BufferAndMemory index;
};

struct SpriteData {
  std::shared_ptr<uint8_t[]> data;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
};

struct ImageBuffer {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

namespace shapes {
struct Shape;
}

namespace sprite {
struct TexturedShape;
}

struct FrameBufferAttachment {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

struct VulkanRenderContext {
  VkCommandBuffer commandBuffer;
};

struct VulkanPipeline {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
};

struct Vertex;
struct TexturedVertex;

class VulkanDevice {
 public:
  VulkanDevice(std::unique_ptr<vk::VulkanInstance> vulkanInstance, uint32_t width, uint32_t height, uint32_t tileSize);
  ~VulkanDevice();

  void initDevice(bool useGpu);
  void initRenderMode(RenderMode mode);

  // Load the sprites
  void preloadSprites(std::unordered_map<std::string, SpriteData>& spritesData);

  // Actual rendering commands
  VulkanRenderContext beginRender();

  ShapeBuffer getShapeBuffer(std::string shapeBufferName);
  void drawShape(VulkanRenderContext& renderContext, ShapeBuffer shapeBuffer, glm::mat4 model, glm::vec3 color);

  uint32_t getSpriteArrayLayer(std::string spriteName);
  void drawSprite(VulkanRenderContext& renderContext, uint32_t arrayLayer, glm::mat4 model, glm::vec3 color);

  std::unique_ptr<uint8_t[]> endRender(VulkanRenderContext& renderContext, std::vector<VkRect2D> dirtyRectangles);

 private:
  std::vector<VkPhysicalDevice> getAvailablePhysicalDevices();
  VulkanPhysicalDeviceInfo getPhysicalDeviceInfo(VkPhysicalDevice& device);
  std::vector<VulkanPhysicalDeviceInfo>::iterator selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices);
  std::vector<VulkanPhysicalDeviceInfo> getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices);
  bool hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices);

  VkCommandBuffer beginCommandBuffer();
  void endCommandBuffer(VkCommandBuffer& commandBuffer);

  std::vector<VkQueueFamilyProperties> getQueueFamilyProperties();

  uint32_t findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

  VkSampler createTextureSampler();

  ImageBuffer createImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat& colorFormat, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
  void copyImage(VkImage imageSrc, VkImage destSrc, std::vector<VkRect2D> rects);
  void copyBufferToImage(VkBuffer bufferSrc, VkImage imageDst, std::vector<VkRect2D> rects, uint32_t arrayLayer);

  ShapeBuffer createSpriteShapeBuffer();
  std::unordered_map<std::string, ShapeBuffer> createShapeBuffers();

  ShapeBuffer createShapeBuffer(shapes::Shape shape);
  ShapeBuffer createTexturedShapeBuffer(sprite::TexturedShape shape);

  void createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data = nullptr);
  
  template<class V>
  BufferAndMemory createVertexBuffers(std::vector<V>& vertices);
  BufferAndMemory createIndexBuffers(std::vector<uint32_t>& vertices);
  void stageToDeviceBuffer(VkBuffer& deviceBuffer, void* data, uint32_t bufferSize);
  void stageToDeviceImage(VkImage& deviceImage, void* data, uint32_t bufferSize, uint32_t arrayLayers);

  FrameBufferAttachment createDepthAttachment();
  FrameBufferAttachment createColorAttachment();
  void createRenderPass();
  VulkanPipeline createShapeRenderPipeline();
  VulkanPipeline createSpriteRenderPipeline();

  void allocateHostImageData();
  void copySceneToHostImage(std::vector<VkRect2D> dirtyRectangles);
  std::unique_ptr<uint8_t[]> copyHostImage();

  void submitCommands(VkCommandBuffer cmdBuffer);

  std::unique_ptr<vk::VulkanInstance> vulkanInstance_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue computeQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;

  FrameBufferAttachment colorAttachment_;
  FrameBufferAttachment depthAttachment_;
  VkFramebuffer frameBuffer_;

  std::unordered_map<std::string, ShapeBuffer> shapeBuffers_;

  // Shape buffer reserved for drawing sprites
  ShapeBuffer spriteShapeBuffer_;

  // An image buffer that stores all of the sprites in an array
  ImageBuffer spriteImageArrayBuffer_;

  // Array indices of sprites that are pre-loaded into a texture array
  std::unordered_map<std::string, uint32_t> spriteIndices_;

  VkRenderPass renderPass_;
  bool isRendering_ = false;

  VulkanPipeline shapeRenderPipeline_;
  VulkanPipeline spriteRenderPipeline_;

  // This is where the rendered image data will be
  VkImage renderedImage_;
  VkDeviceMemory renderedImageMemory_;
  uint8_t* imageRGBA_;
  uint8_t* imageRGB_;

  // Use 8 bit color
  VkFormat colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat_;

  const uint height_;
  const uint width_;
  const uint tileSize_;
  const glm::mat4 ortho_;

};
}  // namespace vk