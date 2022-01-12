#pragma once
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vk {

class VulkanInstance;
class VulkanPhysicalDeviceInfo;
class VulkanQueueFamilyIndices;

enum DeviceSelectionOrder {
  DRIVER_ENUMERATION,  // the order that the devices are returned from the driver (default)
  PCI_BUS_ID           // order by the PCI bus Id ascending
};

struct DeviceSelection {
  std::unordered_set<uint8_t> allowedDeviceIndexes;
  DeviceSelectionOrder order;
};

enum RenderMode {
  SHAPES,
  SPRITES,
};

struct BufferAndMemory {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

struct PersistentSSBOBufferAndMemory {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void* mapped;
};

struct ShapeBuffer {
  size_t indices;
  BufferAndMemory vertex;
  BufferAndMemory index;
};

struct SpriteData {
  std::unique_ptr<uint8_t[]> data;
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
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
};

struct VulkanRenderContext {
  bool isRecording = false;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct VulkanPipeline {
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
  VkSampler sampler = VK_NULL_HANDLE;
};

struct EnvironmentUniform {
  glm::mat4 projectionMatrix{1.0};
  glm::mat4 viewMatrix{1.0};
  glm::vec2 gridDims;
  uint32_t playerId;

  uint32_t globalVariableCount;
  uint32_t objectVariableCount;

  // Deprecated
  uint32_t highlightPlayerObjects;
};

struct PlayerInfoSSBO {
  glm::vec4 playerColor;
};

struct ObjectVariableSSBO {
  int32_t objectVariableValue = 0;
};

struct GlobalVariableSSBO {
  int32_t globalVariableValue;
};

struct ObjectDataSSBO {
  glm::mat4 modelMatrix{1.0};
  glm::vec4 color{1.0};
  glm::vec2 textureMultiply{1.0, 1.0};
  uint32_t textureIndex = 0;
  uint32_t objectTypeId = 0;
  uint32_t playerId = 0;
  int32_t zIdx = 0;
};

struct ObjectSSBOs {
  ObjectDataSSBO objectData;
  std::vector<ObjectVariableSSBO> objectVariables{};
};


struct PersistentSSBOData {
  EnvironmentUniform environmentUniform;
  std::vector<PlayerInfoSSBO> playerInfoSSBOData;
};

struct FrameSSBOData {
  std::vector<GlobalVariableSSBO> globalVariableSSBOData;
  std::vector<ObjectDataSSBO> objectDataSSBOData;
  std::vector<std::vector<ObjectVariableSSBO>> objectVariableSSBOData;
};

struct EnvironmentUniformBuffer {
  PersistentSSBOBufferAndMemory allocated;
  uint32_t allocatedSize = 0;
};

struct PlayerInfoSSBOBuffer {
  PersistentSSBOBufferAndMemory allocated;
  uint32_t count = 0;
  uint32_t paddedSize = 0;
  uint32_t allocatedSize = 0;
};

struct GlobalVariableSSBOBuffer {
  PersistentSSBOBufferAndMemory allocated;
  uint32_t count = 0;
  uint32_t paddedSize = 0;
  uint32_t allocatedSize = 0;
};

struct ObjectDataSSBOBuffer {
  PersistentSSBOBufferAndMemory allocated;
  uint32_t count = 0;
  uint32_t paddedSize = 0;
  uint32_t allocatedSize = 0;
};

struct ObjectVariableSSBOBuffer {
  PersistentSSBOBufferAndMemory allocated;
  uint32_t count = 0;
  uint32_t variableStride = 0;
  uint32_t paddedSize = 0;
  uint32_t allocatedSize = 0;
};

struct Vertex;
struct TexturedVertex;

class VulkanDevice {
 public:
  VulkanDevice(std::shared_ptr<vk::VulkanInstance> vulkanInstance, glm::ivec2 tileSize, std::string resourcePath);
  ~VulkanDevice();

  void initDevice(bool useGpu);
  void initRenderMode(RenderMode mode);
  std::vector<uint32_t> resetRenderSurface(uint32_t pixelWidth, uint32_t pixelHeight);

  // Load the sprites
  void preloadSprites(std::unordered_map<std::string, SpriteData>& spritesData);

  // Setup variables to be passed to the shaders
  void initializeSSBOs(uint32_t globalVariableCount, uint32_t playerCount, uint32_t objectVariableCount, uint32_t maximumObjects);

  // Pass data to shaders before rendering
  void writePersistentSSBOData(PersistentSSBOData& ssboData);
  void writeFrameSSBOData(FrameSSBOData& ssboData);

  // Actual rendering commands
  void startRecordingCommandBuffer();

  ShapeBuffer& getShapeBuffer(std::string shapeBufferName);

  uint32_t getSpriteArrayLayer(std::string spriteName);
  void updateObjectPushConstants(uint32_t objectIndex, ShapeBuffer& shapeBuffers);

  void endRecordingCommandBuffer(std::vector<VkRect2D> dirtyRectangles);
  void executeCommandBuffer(VkCommandBuffer commandBuffer);
  uint8_t* renderFrame();

  bool isInitialized() const;

 private:
  std::vector<VkPhysicalDevice> getAvailablePhysicalDevices();
  VulkanPhysicalDeviceInfo getPhysicalDeviceInfo(VkPhysicalDevice& device);
  std::vector<VulkanPhysicalDeviceInfo>::iterator selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices);
  std::vector<VulkanPhysicalDeviceInfo> getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices);
  bool hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices);

  VkCommandBuffer beginCommandBuffer();

  std::vector<VkQueueFamilyProperties> getQueueFamilyProperties();

  uint32_t findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

  VkSampler createTextureSampler();

  ImageBuffer createImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat& colorFormat, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
  void copyImage(VkCommandBuffer commandBuffer, VkImage imageSrc, VkImage destSrc, std::vector<VkRect2D> rects);
  void copyBufferToImage(VkBuffer bufferSrc, VkImage imageDst, std::vector<VkRect2D> rects, uint32_t arrayLayer);

  ShapeBuffer createSpriteShapeBuffer();
  std::unordered_map<std::string, ShapeBuffer> createShapeBuffers();

  ShapeBuffer createShapeBuffer(shapes::Shape shape);
  ShapeBuffer createTexturedShapeBuffer(sprite::TexturedShape shape);

  void createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data = nullptr);

  template <class T>
  void updateSingleBuffer(std::vector<T> data, uint32_t paddedSize, vk::PersistentSSBOBufferAndMemory bufferAndMemory, uint32_t length=0);

  template <class T> 
  uint32_t calculatedPaddedStructSize(uint32_t minStride);

  template <class V>
  BufferAndMemory createVertexBuffers(std::vector<V>& vertices);
  BufferAndMemory createIndexBuffers(std::vector<uint32_t>& vertices);
  void stageToDeviceBuffer(VkBuffer& deviceBuffer, void* data, VkDeviceSize bufferSize);
  void stageToDeviceImage(VkImage& deviceImage, void* data, VkDeviceSize bufferSize, uint32_t arrayLayers);

  FrameBufferAttachment createDepthAttachment();
  FrameBufferAttachment createColorAttachment();
  void createRenderPass();
  VulkanPipeline createShapeRenderPipeline();
  VulkanPipeline createSpriteRenderPipeline();

  std::vector<uint32_t> allocateHostImageData();

  void freeRenderSurfaceMemory();

  DeviceSelection getAllowedGPUIdxs() const;

  std::shared_ptr<vk::VulkanInstance> vulkanInstance_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue computeQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;

  FrameBufferAttachment colorAttachment_;
  FrameBufferAttachment depthAttachment_;
  VkFramebuffer frameBuffer_ = VK_NULL_HANDLE;

  std::unordered_map<std::string, ShapeBuffer> shapeBuffers_;

  // Shape buffer reserved for drawing sprites
  ShapeBuffer spriteShapeBuffer_;

  // An image buffer that stores all of the sprites in an array
  ImageBuffer spriteImageArrayBuffer_;

  PlayerInfoSSBOBuffer playerInfoSSBOBuffer_;
  EnvironmentUniformBuffer environmentUniformBuffer_;
  GlobalVariableSSBOBuffer globalVariableSSBOBuffer_;
  ObjectDataSSBOBuffer objectDataSSBOBuffer_;
  ObjectVariableSSBOBuffer objectVariableSSBOBuffer_;

  // Array indices of sprites that are pre-loaded into a texture array
  std::unordered_map<std::string, uint32_t> spriteIndices_;

  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VulkanRenderContext renderContext_;

  RenderMode renderMode_;
  VulkanPipeline renderPipeline_;

  // This is where the rendered image data will be
  VkImage renderedImage_ = VK_NULL_HANDLE;
  VkDeviceMemory renderedImageMemory_ = VK_NULL_HANDLE;
  uint8_t* imageRGBA_;
  //std::shared_ptr<uint8_t> imageRGB_;

  // Use 8 bit color
  VkFormat colorFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat_;

  uint32_t width_;
  uint32_t height_;

  const glm::ivec2 tileSize_;

  const std::string shaderPath_;

  bool isInitialized_ = false;
};
}  // namespace vk