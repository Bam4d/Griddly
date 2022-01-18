#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <sstream>

#include "ShapeBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanInstance.hpp"
#include "VulkanPhysicalDeviceInfo.hpp"
#include "VulkanQueueFamilyIndices.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanDevice::VulkanDevice(std::shared_ptr<vk::VulkanInstance> vulkanInstance, glm::ivec2 tileSize, std::string shaderPath)
    : vulkanInstance_(vulkanInstance),
      tileSize_(tileSize),
      shaderPath_(shaderPath) {
}

VulkanDevice::~VulkanDevice() {
  if (device_ != VK_NULL_HANDLE) {
    // Free all the vertex/index buffers
    vkDestroyBuffer(device_, shapeBuffer_.vertex.buffer, NULL);
    vkFreeMemory(device_, shapeBuffer_.vertex.memory, NULL);
    vkDestroyBuffer(device_, shapeBuffer_.index.buffer, NULL);
    vkFreeMemory(device_, shapeBuffer_.index.memory, NULL);

    freeRenderSurfaceMemory();

    // Destroy sprite images
    vkDestroyImage(device_, spriteImageArrayBuffer_.image, NULL);
    vkFreeMemory(device_, spriteImageArrayBuffer_.memory, NULL);
    vkDestroyImageView(device_, spriteImageArrayBuffer_.view, NULL);

    // Destroy shader buffers
    vkDestroyBuffer(device_, environmentUniformBuffer_.allocated.buffer, NULL);
    vkUnmapMemory(device_, environmentUniformBuffer_.allocated.memory);
    vkFreeMemory(device_, environmentUniformBuffer_.allocated.memory, NULL);

    vkDestroyBuffer(device_, playerInfoSSBOBuffer_.allocated.buffer, NULL);
    vkUnmapMemory(device_, playerInfoSSBOBuffer_.allocated.memory);
    vkFreeMemory(device_, playerInfoSSBOBuffer_.allocated.memory, NULL);

    vkDestroyBuffer(device_, objectDataSSBOBuffer_.allocated.buffer, NULL);
    vkUnmapMemory(device_, objectDataSSBOBuffer_.allocated.memory);
    vkFreeMemory(device_, objectDataSSBOBuffer_.allocated.memory, NULL);

    if (globalVariableSSBOBuffer_.allocatedSize > 0) {
      vkDestroyBuffer(device_, globalVariableSSBOBuffer_.allocated.buffer, NULL);
      vkUnmapMemory(device_, globalVariableSSBOBuffer_.allocated.memory);
      vkFreeMemory(device_, globalVariableSSBOBuffer_.allocated.memory, NULL);
    }

    if (objectVariableSSBOBuffer_.allocatedSize > 0) {
      vkDestroyBuffer(device_, objectVariableSSBOBuffer_.allocated.buffer, NULL);
      vkUnmapMemory(device_, objectVariableSSBOBuffer_.allocated.memory);
      vkFreeMemory(device_, objectVariableSSBOBuffer_.allocated.memory, NULL);
    }

    vkDestroyCommandPool(device_, commandPool_, NULL);
    vkDestroyDevice(device_, NULL);
  }
}

void VulkanDevice::freeRenderSurfaceMemory() {
  // Remove frame buffers
  if (colorAttachment_.image != VK_NULL_HANDLE) {
    vkDestroyImage(device_, colorAttachment_.image, NULL);
    vkFreeMemory(device_, colorAttachment_.memory, NULL);
    vkDestroyImageView(device_, colorAttachment_.view, NULL);
  }

  if (depthAttachment_.image != VK_NULL_HANDLE) {
    vkDestroyImage(device_, depthAttachment_.image, NULL);
    vkFreeMemory(device_, depthAttachment_.memory, NULL);
    vkDestroyImageView(device_, depthAttachment_.view, NULL);
  }

  if (frameBuffer_ != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device_, frameBuffer_, NULL);
  }

  if (renderPass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_, renderPass_, NULL);
  }

  // Remove the rendering surface
  if (renderedImage_ != VK_NULL_HANDLE) {
    vkDestroyImage(device_, renderedImage_, NULL);
  }

  if (renderedImageMemory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, renderedImageMemory_, NULL);
  }

  vkDestroyPipeline(device_, renderPipeline_.pipeline, NULL);
  vkDestroyDescriptorPool(device_, renderPipeline_.descriptorPool, NULL);
  vkDestroyPipelineLayout(device_, renderPipeline_.pipelineLayout, NULL);
  vkDestroyDescriptorSetLayout(device_, renderPipeline_.descriptorSetLayout, NULL);

  for (auto& shader : renderPipeline_.shaderStages) {
    vkDestroyShaderModule(device_, shader.module, NULL);
  }

  vkDestroySampler(device_, renderPipeline_.sampler, NULL);

}  // namespace vk

void VulkanDevice::initDevice(bool useGPU) {
  spdlog::debug("Initializing Vulkan Device.");
  std::vector<VkPhysicalDevice> physicalDevices = getAvailablePhysicalDevices();
  std::vector<VulkanPhysicalDeviceInfo> supportedPhysicalDevices = getSupportedPhysicalDevices(physicalDevices);

  if (supportedPhysicalDevices.size() > 0) {
    auto physicalDeviceInfo = &supportedPhysicalDevices[0];

    spdlog::info("Using device \"{0}\" for rendering.", physicalDeviceInfo->deviceName);

    auto graphicsQueueFamilyIndex = physicalDeviceInfo->queueFamilyIndices.graphicsIndices;
    auto computeQueueFamilyIndex = physicalDeviceInfo->queueFamilyIndices.computeIndices;

    auto deviceQueueCreateInfo = vk::initializers::deviceQueueCreateInfo(graphicsQueueFamilyIndex, 1.0f);
    auto deviceCreateInfo = vk::initializers::deviceCreateInfo(deviceQueueCreateInfo);

    physicalDevice_ = physicalDeviceInfo->physicalDevice;
    spdlog::debug("Creating physical device.");
    vk_check(vkCreateDevice(physicalDevice_, &deviceCreateInfo, NULL, &device_));
    vkGetDeviceQueue(device_, computeQueueFamilyIndex, 0, &computeQueue_);

    spdlog::debug("Creating command pool.");
    auto commandPoolCreateInfo = vk::initializers::commandPoolCreateInfo(computeQueueFamilyIndex);
    vk_check(vkCreateCommandPool(device_, &commandPoolCreateInfo, NULL, &commandPool_));

  } else {
    spdlog::error("No devices supporting vulkan present for rendering.");
  }

  shapeBuffer_ = createSpriteShapeBuffer();

  isInitialized_ = true;
}

bool VulkanDevice::isInitialized() const {
  return isInitialized_;
}

std::vector<uint32_t> VulkanDevice::resetRenderSurface(uint32_t pixelWidth, uint32_t pixelHeight) {
  freeRenderSurfaceMemory();

  height_ = pixelHeight;
  width_ = pixelWidth;

  spdlog::debug("Creating colour frame buffer.");
  colorAttachment_ = createColorAttachment();
  spdlog::debug("Creating depth frame buffer.");
  depthAttachment_ = createDepthAttachment();

  spdlog::debug("Creating render pass.");
  createRenderPass();

  spdlog::debug("Allocating offscreen host image data.");
  auto imageStrides = allocateHostImageData();

  renderPipeline_ = createSpriteRenderPipeline();

  spdlog::debug("Render Surface Strides ({0}, {1}, {2}).", imageStrides[0], imageStrides[1], imageStrides[2]);
  return imageStrides;
}

VkCommandBuffer VulkanDevice::beginCommandBuffer() {
  VkCommandBuffer commandBuffer;

  VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::initializers::commandBufferAllocateInfo(commandPool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  vk_check(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::commandBufferBeginInfo();
  vk_check(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

  return commandBuffer;
}

void VulkanDevice::startRecordingCommandBuffer() {
  assert(("Cannot begin a recording session if already recording.", !renderContext_.isRecording));

  if (renderContext_.commandBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(device_, commandPool_, 1, &renderContext_.commandBuffer);
  }

  vk_check(vkResetCommandPool(device_, commandPool_, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

  renderContext_.isRecording = true;

  renderContext_.commandBuffer = beginCommandBuffer();

  VkClearValue clearValues[2];
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderArea.extent.width = width_;
  renderPassBeginInfo.renderArea.extent.height = height_;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;
  renderPassBeginInfo.renderPass = renderPass_;
  renderPassBeginInfo.framebuffer = frameBuffer_;

  vkCmdBeginRenderPass(renderContext_.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.height = static_cast<float>(height_);
  viewport.width = static_cast<float>(width_);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(renderContext_.commandBuffer, 0, 1, &viewport);

  // Update dynamic scissor state
  VkRect2D scissor = {};
  scissor.extent.width = width_;
  scissor.extent.height = height_;
  vkCmdSetScissor(renderContext_.commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(renderContext_.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline_.pipelineLayout, 0, 1, &renderPipeline_.descriptorSet, 0, NULL);
  vkCmdBindPipeline(renderContext_.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline_.pipeline);
}

uint32_t VulkanDevice::getSpriteArrayLayer(std::string spriteName) {
  return spriteIndices_.at(spriteName);
}

void VulkanDevice::updateObjectPushConstants(uint32_t objectIndex) {
  ObjectPushConstants objectPushConstants = {objectIndex};
  const VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(renderContext_.commandBuffer, 0, 1, &shapeBuffer_.vertex.buffer, offsets);
  vkCmdBindIndexBuffer(renderContext_.commandBuffer, shapeBuffer_.index.buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdPushConstants(renderContext_.commandBuffer, renderPipeline_.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectPushConstants), &objectPushConstants);
  vkCmdDrawIndexed(renderContext_.commandBuffer, shapeBuffer_.indices, 1, 0, 0, 0);
}

void VulkanDevice::endRecordingCommandBuffer(std::vector<VkRect2D> dirtyRectangles = {}) {
  vkCmdEndRenderPass(renderContext_.commandBuffer);

  copyImage(renderContext_.commandBuffer, colorAttachment_.image, renderedImage_, dirtyRectangles);

  vk_check(vkEndCommandBuffer(renderContext_.commandBuffer));

  renderContext_.isRecording = false;
}

void VulkanDevice::copyBufferToImage(VkBuffer bufferSrc, VkImage imageDst, std::vector<VkRect2D> rects, uint32_t arrayLayer) {
  auto commandBuffer = beginCommandBuffer();

  auto numRects = rects.size();

  //Image barrier stuff
  vk::insertImageMemoryBarrier(
      commandBuffer,
      imageDst,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, arrayLayer, 1});

  std::vector<VkBufferImageCopy> imageCopyRegions;
  for (auto& rect : rects) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = arrayLayer;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {rect.offset.x, rect.offset.y, 0};
    region.imageExtent = {rect.extent.width, rect.extent.height, 1};

    imageCopyRegions.push_back(region);
  }

  vkCmdCopyBufferToImage(
      commandBuffer,
      bufferSrc,
      imageDst,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      numRects,
      imageCopyRegions.data());

  vk::insertImageMemoryBarrier(
      commandBuffer,
      imageDst,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, arrayLayer, 1});

  vk_check(vkEndCommandBuffer(commandBuffer));
  executeCommandBuffer(commandBuffer);
  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanDevice::copyImage(VkCommandBuffer commandBuffer, VkImage imageSrc, VkImage imageDst, std::vector<VkRect2D> rects) {
  //VkCommandBuffer commandBuffer = beginCommandBuffer();

  auto numRects = rects.size();

  if (numRects > 0) {
    // Transition destination image to transfer destination layout
    vk::insertImageMemoryBarrier(
        commandBuffer,
        imageDst,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    std::vector<VkImageCopy> imageCopyRegions;

    for (auto& rect : rects) {
      VkImageCopy imageCopyRegion{};
      imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.srcSubresource.layerCount = 1;
      imageCopyRegion.srcOffset.x = rect.offset.x;
      imageCopyRegion.srcOffset.y = rect.offset.y;
      imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.dstSubresource.layerCount = 1;

      imageCopyRegion.dstOffset.x = rect.offset.x;
      imageCopyRegion.dstOffset.y = rect.offset.y;
      imageCopyRegion.extent.height = rect.extent.height;
      imageCopyRegion.extent.width = rect.extent.width;
      imageCopyRegion.extent.depth = 1;

      imageCopyRegions.push_back(imageCopyRegion);
    }

    vkCmdCopyImage(
        commandBuffer,
        imageSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        imageDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        numRects,
        imageCopyRegions.data());

    // Transition destination image to general layout, which is the required layout for mapping the image memory later on
    vk::insertImageMemoryBarrier(
        commandBuffer,
        imageDst,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
  }
}

std::vector<uint32_t> VulkanDevice::allocateHostImageData() {
  // Create the linear tiled destination image to copy to and to read the memory from

  auto imageBuffer = createImage(width_, height_, 1, colorFormat_, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

  renderedImage_ = imageBuffer.image;
  renderedImageMemory_ = imageBuffer.memory;

  // Map image memory so we can start copying from it
  vkMapMemory(device_, renderedImageMemory_, 0, VK_WHOLE_SIZE, 0, (void**)&imageRGBA_);
  // imageRGB_ = std::shared_ptr<uint8_t>(new uint8_t[width_ * height_ * 4](), std::default_delete<uint8_t[]>());

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device_, renderedImage_, &subResource, &subResourceLayout);

  imageRGBA_ += subResourceLayout.offset;

  return {1, 4, (uint32_t)subResourceLayout.rowPitch};
}

void VulkanDevice::preloadSprites(std::unordered_map<std::string, SpriteData>& spritesData) {
  auto arrayLayers = spritesData.size();

  spdlog::debug("Preloading {0} sprites", arrayLayers);

  spriteImageArrayBuffer_ = createImage(tileSize_.x, tileSize_.y, arrayLayers, colorFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageViewCreateInfo spriteImageView = vk::initializers::imageViewCreateInfo(colorFormat_, spriteImageArrayBuffer_.image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, arrayLayers);
  vk_check(vkCreateImageView(device_, &spriteImageView, NULL, &spriteImageArrayBuffer_.view));

  int layer = 0;
  for (auto& spriteToLoad : spritesData) {
    auto& spriteInfo = spriteToLoad.second;
    auto spriteName = spriteToLoad.first;

    VkDeviceSize spriteSize = spriteInfo.width * spriteInfo.height * spriteInfo.channels;

    auto imageData = spriteInfo.data.get();
    stageToDeviceImage(spriteImageArrayBuffer_.image, imageData, spriteSize, layer);
    spriteIndices_.insert({spriteName, layer});
    layer++;
  }
}

void VulkanDevice::initializeSSBOs(uint32_t globalVariableCount, uint32_t playerCount, uint32_t objectVariableCount, uint32_t maximumObjects) {
  spdlog::debug("Initializing environment uniform buffer.");
  environmentUniformBuffer_.allocatedSize = sizeof(EnvironmentUniform);
  createBuffer(
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      &environmentUniformBuffer_.allocated.buffer,
      &environmentUniformBuffer_.allocated.memory,
      environmentUniformBuffer_.allocatedSize);
  vk_check(vkMapMemory(device_, environmentUniformBuffer_.allocated.memory, 0, environmentUniformBuffer_.allocatedSize, 0, &environmentUniformBuffer_.allocated.mapped));

  spdlog::debug("Initializing player info SSBO with max {0} objects", playerCount);
  playerInfoSSBOBuffer_.count = playerCount;
  playerInfoSSBOBuffer_.paddedSize = calculatedPaddedStructSize<PlayerInfoSSBO>(16);
  playerInfoSSBOBuffer_.allocatedSize = playerInfoSSBOBuffer_.paddedSize * playerInfoSSBOBuffer_.count;
  createBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      &playerInfoSSBOBuffer_.allocated.buffer,
      &playerInfoSSBOBuffer_.allocated.memory,
      playerInfoSSBOBuffer_.allocatedSize);
  vk_check(vkMapMemory(device_, playerInfoSSBOBuffer_.allocated.memory, 0, playerInfoSSBOBuffer_.allocatedSize, 0, &playerInfoSSBOBuffer_.allocated.mapped));

  spdlog::debug("Initializing object data SSBO with max {0} objects", maximumObjects);
  objectDataSSBOBuffer_.count = maximumObjects;
  objectDataSSBOBuffer_.paddedSize = calculatedPaddedStructSize<ObjectDataSSBO>(16);
  objectDataSSBOBuffer_.allocatedSize = 16 + objectDataSSBOBuffer_.paddedSize * objectDataSSBOBuffer_.count;
  createBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      &objectDataSSBOBuffer_.allocated.buffer,
      &objectDataSSBOBuffer_.allocated.memory,
      objectDataSSBOBuffer_.allocatedSize);
  vk_check(vkMapMemory(device_, objectDataSSBOBuffer_.allocated.memory, 0, objectDataSSBOBuffer_.allocatedSize, 0, &objectDataSSBOBuffer_.allocated.mapped));

  globalVariableCount_ = globalVariableCount;
  if (globalVariableCount > 0) {
    spdlog::debug("Initializing global variable SSBO with {0} variables", globalVariableCount);
    globalVariableSSBOBuffer_.count = globalVariableCount;
    globalVariableSSBOBuffer_.paddedSize = calculatedPaddedStructSize<GlobalVariableSSBO>(4);
    globalVariableSSBOBuffer_.allocatedSize = globalVariableSSBOBuffer_.paddedSize * globalVariableSSBOBuffer_.count;
    createBuffer(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &globalVariableSSBOBuffer_.allocated.buffer,
        &globalVariableSSBOBuffer_.allocated.memory,
        globalVariableSSBOBuffer_.allocatedSize);
    vk_check(vkMapMemory(device_, globalVariableSSBOBuffer_.allocated.memory, 0, globalVariableSSBOBuffer_.allocatedSize, 0, &globalVariableSSBOBuffer_.allocated.mapped));
  }

  objectVariableCount_ = objectVariableCount;
  if (objectVariableCount > 0) {
    spdlog::debug("Initializing object variable SSBO with max {0} objects, {1} variables. ", maximumObjects, objectVariableCount);
    objectVariableSSBOBuffer_.count = maximumObjects * objectVariableCount;
    objectVariableSSBOBuffer_.variableStride = objectVariableCount;
    objectVariableSSBOBuffer_.paddedSize = calculatedPaddedStructSize<ObjectVariableSSBO>(4);
    objectVariableSSBOBuffer_.allocatedSize = objectVariableSSBOBuffer_.paddedSize * objectVariableSSBOBuffer_.count;
    createBuffer(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &objectVariableSSBOBuffer_.allocated.buffer,
        &objectVariableSSBOBuffer_.allocated.memory,
        objectVariableSSBOBuffer_.allocatedSize);

    vk_check(vkMapMemory(device_, objectVariableSSBOBuffer_.allocated.memory, 0, objectVariableSSBOBuffer_.allocatedSize, 0, &objectVariableSSBOBuffer_.allocated.mapped));
  }
}

template <class T>
uint32_t VulkanDevice::calculatedPaddedStructSize(uint32_t minStride) {
  uint32_t paddedStructSize = 0;
  for (int i = minStride; i < sizeof(T) + minStride; i += minStride) {
    paddedStructSize = i;
  }
  return paddedStructSize;
}

template <class T>
void VulkanDevice::updateContiguousBuffer(std::vector<T> data, uint32_t paddedDataSize, vk::PersistentSSBOBufferAndMemory bufferAndMemory, uint32_t length) {
  auto lengthOffset = 0;
  // Place a length value at the beginning
  if (length > 0) {
    lengthOffset = 16;
  }

  if (length > 0) {
    memcpy((static_cast<char*>(bufferAndMemory.mapped)), &length, sizeof(length));
  }

  for (int i = 0; i < data.size(); i++) {
    auto offset = i * paddedDataSize + lengthOffset;
    memcpy((static_cast<char*>(bufferAndMemory.mapped) + offset), &data[i], paddedDataSize);
  }
}

void VulkanDevice::updateObjectBuffer(FrameSSBOData& ssboData) {
  uint32_t length = ssboData.objectSSBOData.size();
  spdlog::debug("Updating object data storage buffer. {0} objects. padded object size: {1}. update size {2}", length, objectDataSSBOBuffer_.paddedSize, length * objectDataSSBOBuffer_.paddedSize);

  // Place a length value at the beginning
  auto lengthOffset = 16;
  auto paddedDataSize = objectDataSSBOBuffer_.paddedSize;
  auto& bufferAndMemory = objectDataSSBOBuffer_.allocated;
  auto& objectDataCache = ssboData.objectSSBOData;

  memcpy((static_cast<char*>(bufferAndMemory.mapped)), &length, sizeof(length));

  for (int i = 0; i < length; i++) {
    auto offset = i * paddedDataSize + lengthOffset;
    auto& objectSSBOs = objectDataCache[i];

    memcpy((static_cast<char*>(bufferAndMemory.mapped) + offset), &objectSSBOs.objectData, paddedDataSize);
  }
}

void VulkanDevice::updateObjectVariableBuffer(FrameSSBOData& ssboData) {
  uint32_t length = ssboData.objectSSBOData.size();
  spdlog::debug("Updating object variable storage buffer. {0} objects. padded variable size: {1}. update size {2}", length, objectVariableSSBOBuffer_.paddedSize, length * objectVariableSSBOBuffer_.paddedSize);

  auto& bufferAndMemory = objectVariableSSBOBuffer_.allocated;
  auto& objectDataCache = ssboData.objectSSBOData;
  auto paddedDataSize = objectVariableSSBOBuffer_.paddedSize;
  auto variableStride = objectVariableSSBOBuffer_.variableStride;

  for (int i = 0; i < length; i++) {
    auto& objectVariables = objectDataCache[i].objectVariables;
    for (int j = 0; j < objectVariables.size(); j++) {
      auto offset = (i * variableStride + j) * paddedDataSize;
      memcpy((static_cast<char*>(bufferAndMemory.mapped) + offset), &objectVariables[j], paddedDataSize);
    }
  }

}  // namespace vk

void VulkanDevice::writePersistentSSBOData(PersistentSSBOData& ssboData) {
  // Copy environment data
  spdlog::debug("Updating environment data uniform buffer. size: {0}", environmentUniformBuffer_.allocatedSize);
  updateContiguousBuffer(std::vector{ssboData.environmentUniform}, environmentUniformBuffer_.allocatedSize, environmentUniformBuffer_.allocated);

  // Copy all player data
  spdlog::debug("Updating player info storage buffer. {0} objects. padded object size: {1}. update size {2}", ssboData.playerInfoSSBOData.size(), playerInfoSSBOBuffer_.paddedSize, ssboData.playerInfoSSBOData.size() * playerInfoSSBOBuffer_.paddedSize);
  updateContiguousBuffer(ssboData.playerInfoSSBOData, playerInfoSSBOBuffer_.paddedSize, playerInfoSSBOBuffer_.allocated);
}

void VulkanDevice::writeFrameSSBOData(FrameSSBOData& ssboData) {
  // Copy global data if its available
  spdlog::debug("Updating global variable storage buffer. {0} variables. padded variable size: {1}. update size {2}", globalVariableSSBOBuffer_.count, globalVariableSSBOBuffer_.paddedSize, ssboData.globalVariableSSBOData.size() * globalVariableSSBOBuffer_.paddedSize);
  if (globalVariableCount_ > 0) {
    updateContiguousBuffer(ssboData.globalVariableSSBOData, globalVariableSSBOBuffer_.paddedSize, globalVariableSSBOBuffer_.allocated);
  }

  // Copy all object data
  updateObjectBuffer(ssboData);

  // Copy Object Variables
  if (objectVariableCount_ > 0) {
    updateObjectVariableBuffer(ssboData);
  }
}

VkSampler VulkanDevice::createTextureSampler() {
  VkSampler textureSampler;
  auto samplerCreateInfo = vk::initializers::samplerCreateInfo();

  spdlog::debug("Creating texture sampler");

  vk_check(vkCreateSampler(device_, &samplerCreateInfo, NULL, &textureSampler));
  return textureSampler;
}

ShapeBuffer VulkanDevice::createSpriteShapeBuffer() {
  auto shape = sprite::squareSprite;

  // Vertex Buffers
  auto vertexBuffer = createVertexBuffers(shape.vertices);

  // Index Buffers
  auto indexBuffer = createIndexBuffers(shape.indices);

  return {shape.indices.size(), vertexBuffer, indexBuffer};
}

template <class V>
BufferAndMemory VulkanDevice::createVertexBuffers(std::vector<V>& vertices) {
  const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(V);

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexMemory;

  spdlog::debug("Creating vertex buffer.");
  createBuffer(
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &vertexBuffer,
      &vertexMemory,
      vertexBufferSize);

  stageToDeviceBuffer(vertexBuffer, vertices.data(), vertexBufferSize);

  return {vertexBuffer, vertexMemory};
}

BufferAndMemory VulkanDevice::createIndexBuffers(std::vector<uint32_t>& indices) {
  const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

  VkBuffer indexBuffer;
  VkDeviceMemory indexMemory;

  spdlog::debug("Creating index buffer.");
  createBuffer(
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &indexBuffer,
      &indexMemory,
      indexBufferSize);

  stageToDeviceBuffer(indexBuffer, indices.data(), indexBufferSize);

  return {indexBuffer, indexMemory};
}

void VulkanDevice::stageToDeviceBuffer(VkBuffer& deviceBuffer, void* data, VkDeviceSize bufferSize) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  spdlog::debug("Creating staging memory buffers to transfer {0} bytes.", bufferSize);
  createBuffer(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory,
      bufferSize,
      data);

  auto commandBuffer = beginCommandBuffer();

  VkBufferCopy copyRegion = {};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceBuffer, 1, &copyRegion);

  vk_check(vkEndCommandBuffer(commandBuffer));
  executeCommandBuffer(commandBuffer);
  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);

  vkDestroyBuffer(device_, stagingBuffer, NULL);
  vkFreeMemory(device_, stagingMemory, NULL);

  spdlog::debug("Done!");
}

void VulkanDevice::stageToDeviceImage(VkImage& deviceImage, void* data, VkDeviceSize bufferSize, uint32_t arrayLayer) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  spdlog::debug("Creating staging memory buffers to transfer {0} bytes.", bufferSize);
  createBuffer(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory,
      bufferSize,
      data);

  copyBufferToImage(stagingBuffer, deviceImage, {{{0, 0}, {(uint32_t)tileSize_.x, (uint32_t)tileSize_.y}}}, arrayLayer);

  vkDestroyBuffer(device_, stagingBuffer, NULL);
  vkFreeMemory(device_, stagingMemory, NULL);

  spdlog::debug("Done!");
}

void VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data) {
  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo = vk::initializers::bufferCreateInfo(usageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vk_check(vkCreateBuffer(device_, &bufferCreateInfo, NULL, buffer));

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = vk::initializers::memoryAllocateInfo();
  vkGetBufferMemoryRequirements(device_, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
  vk_check(vkAllocateMemory(device_, &memAlloc, NULL, memory));

  // Initial memory allocation
  if (data != NULL) {
    void* mapped;
    vk_check(vkMapMemory(device_, *memory, 0, size, 0, &mapped));
    memcpy(mapped, data, size);
    vkUnmapMemory(device_, *memory);
  }

  vk_check(vkBindBufferMemory(device_, *buffer, *memory, 0));
}

uint32_t VulkanDevice::findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &deviceMemoryProperties);
  for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    typeBits >>= 1;
  }
  spdlog::error("Could not find memory type!");
  return 0;
}

std::vector<VulkanPhysicalDeviceInfo>::iterator VulkanDevice::selectPhysicalDevice(bool useGpu, std::vector<VulkanPhysicalDeviceInfo>& supportedDevices) {
  for (auto it = supportedDevices.begin(); it != supportedDevices.end(); ++it) {
    if (useGpu == it->isGpu) {
      return it;
    }
  }
  return supportedDevices.end();
}

DeviceSelection VulkanDevice::getAllowedGPUIdxs() const {
  DeviceSelectionOrder deviceSelectionOrder;
  if (const char* gpuIdxOrder = std::getenv("GRIDDLY_DEVICE_ORDER")) {
    auto gpuIdxOrderString = std::string(gpuIdxOrder);
    if (gpuIdxOrderString == "PCI_BUS_ID") {
      deviceSelectionOrder = DeviceSelectionOrder::PCI_BUS_ID;
      spdlog::debug("GRIDDLY_DEVICE_ORDER: PCI_BUS_ID");
    } else {
      deviceSelectionOrder = DeviceSelectionOrder::DRIVER_ENUMERATION;
      spdlog::debug("GRIDDLY_DEVICE_ORDER: DRIVER_ENUMERATION");
    }

  } else {
    deviceSelectionOrder = DeviceSelectionOrder::DRIVER_ENUMERATION;
  }

  std::unordered_set<uint8_t> gpuIdxs = {};
  if (const char* gpuIdxList = std::getenv("GRIDDLY_VISIBLE_DEVICES")) {
    //parse the indexes here
    try {
      auto end = gpuIdxList + std::strlen(gpuIdxList);
      if (std::find(gpuIdxList, end, ',') != end) {
        auto gpuIdxListString = std::istringstream(gpuIdxList);
        std::string out;
        while (std::getline(gpuIdxListString, out, ',')) {
          auto visibleDeviceIdx = (uint8_t)atoi(out.c_str());
          spdlog::debug("Adding GRIDDLY_VISIBLE_DEVICE: {0}", visibleDeviceIdx);
          gpuIdxs.insert(visibleDeviceIdx);
        }
      } else {
        auto visibleDeviceIdx = (uint8_t)atoi(gpuIdxList);
        spdlog::debug("Adding GRIDDLY_VISIBLE_DEVICE: {0}", visibleDeviceIdx);
        gpuIdxs.insert(visibleDeviceIdx);
      }
    } catch (std::exception e) {
      spdlog::error("Invalid value for GRIDDLY_VISIBLE_DEVICES ({0}). Should be a single integer or a comma seperated list of integers e.g \"0,1\".", gpuIdxList);
    }
  }

  return DeviceSelection{gpuIdxs, deviceSelectionOrder};
}

std::vector<VulkanPhysicalDeviceInfo> VulkanDevice::getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices) {
  // This GPU ID needs to coincide with the GPU Id that cuda uses.
  uint8_t gpuIdx = 0;

  auto deviceSelection = getAllowedGPUIdxs();

  bool limitGpuUsage = deviceSelection.allowedDeviceIndexes.size() > 0;
  auto allowedGpuIdx = deviceSelection.allowedDeviceIndexes;

  std::vector<VulkanPhysicalDeviceInfo> supportedPhysicalDeviceList;
  std::vector<VulkanPhysicalDeviceInfo> physicalDeviceInfoList;

  for (auto& physicalDevice : physicalDevices) {
    physicalDeviceInfoList.push_back(getPhysicalDeviceInfo(physicalDevice));
  }

  if (deviceSelection.order == DeviceSelectionOrder::PCI_BUS_ID) {
    spdlog::debug("Sorting devices by PCI_BUS_ID ascending");
    std::sort(physicalDeviceInfoList.begin(), physicalDeviceInfoList.end(), [](const VulkanPhysicalDeviceInfo& a, const VulkanPhysicalDeviceInfo& b) -> bool { return a.pciBusId < b.pciBusId; });
  }

  for (auto& physicalDeviceInfo : physicalDeviceInfoList) {
    spdlog::debug("Device {0}, isGpu {1}, PCI bus: {2}, isSupported {3}.", physicalDeviceInfo.deviceName, physicalDeviceInfo.isGpu, physicalDeviceInfo.pciBusId, physicalDeviceInfo.isSupported);
    if (physicalDeviceInfo.isGpu) {
      physicalDeviceInfo.gpuIdx = gpuIdx++;
    }

    if (physicalDeviceInfo.isSupported) {
      if (physicalDeviceInfo.isGpu && limitGpuUsage) {
        if (allowedGpuIdx.find(physicalDeviceInfo.gpuIdx) != allowedGpuIdx.end()) {
          spdlog::debug("GPU Device {0}, Id: {1}, PCI bus: {2} -> Visible", physicalDeviceInfo.deviceName, physicalDeviceInfo.gpuIdx, physicalDeviceInfo.pciBusId);
          supportedPhysicalDeviceList.push_back(physicalDeviceInfo);
        }
      } else {
        supportedPhysicalDeviceList.push_back(physicalDeviceInfo);
      }
    }
  }

  return supportedPhysicalDeviceList;
}

std::vector<VkPhysicalDevice> VulkanDevice::getAvailablePhysicalDevices() {
  uint32_t deviceCount = 0;
  vk_check(vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, NULL));
  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  vk_check(vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, physicalDevices.data()));

  return physicalDevices;
}

VulkanPhysicalDeviceInfo VulkanDevice::getPhysicalDeviceInfo(VkPhysicalDevice& physicalDevice) {
  VulkanQueueFamilyIndices queueFamilyIndices;

  VkPhysicalDevicePCIBusInfoPropertiesEXT devicePCIBusInfo{};
  devicePCIBusInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;

  VkPhysicalDeviceProperties2 deviceProperties2 = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      &devicePCIBusInfo};

  vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

  auto deviceProperties = deviceProperties2.properties;

  auto deviceName = deviceProperties.deviceName;

  spdlog::debug("Device found {0}, PCI Bus: {1}. checking for Vulkan support...", deviceName, devicePCIBusInfo.pciBus);

  bool isGpu = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  bool isSupported = hasQueueFamilySupport(physicalDevice, queueFamilyIndices);

  uint8_t pciBusId = devicePCIBusInfo.pciBus;

  return {
      physicalDevice,
      std::string(deviceName),
      isGpu,
      isSupported,
      0,
      pciBusId,
      queueFamilyIndices};
}

bool VulkanDevice::hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices) {
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

  uint32_t i = 0;
  for (auto& queueFamily : queueFamilyProperties) {
    if (queueFamily.queueCount > 0) {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queueFamilyIndices.graphicsIndices = i;
      }

      if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        queueFamilyIndices.computeIndices = i;
      }
    }

    if (queueFamilyIndices.graphicsIndices < UINT32_MAX && queueFamilyIndices.computeIndices < UINT32_MAX) {
      return true;
    }
    i++;
  }
  return false;
}

FrameBufferAttachment VulkanDevice::createDepthAttachment() {
  FrameBufferAttachment depthAttachment;

  getSupportedDepthFormat(physicalDevice_, &depthFormat_);

  auto imageBuffer = createImage(
      width_,
      height_,
      1,
      depthFormat_,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  depthAttachment.image = imageBuffer.image;
  depthAttachment.memory = imageBuffer.memory;

  VkImageViewCreateInfo depthStencilView = vk::initializers::imageViewCreateInfo(depthFormat_, depthAttachment.image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
  vk_check(vkCreateImageView(device_, &depthStencilView, NULL, &depthAttachment.view));

  return depthAttachment;
}

FrameBufferAttachment VulkanDevice::createColorAttachment() {
  FrameBufferAttachment colorAttachment;

  auto imageBuffer = createImage(
      width_,
      height_,
      1,
      colorFormat_,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  colorAttachment.image = imageBuffer.image;
  colorAttachment.memory = imageBuffer.memory;

  VkImageViewCreateInfo colorImageView = vk::initializers::imageViewCreateInfo(colorFormat_, colorAttachment.image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
  vk_check(vkCreateImageView(device_, &colorImageView, NULL, &colorAttachment.view));

  return colorAttachment;
}

ImageBuffer VulkanDevice::createImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat& format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
  VkImage image;
  VkDeviceMemory memory;

  VkImageCreateInfo imageInfo = vk::initializers::imageCreateInfo(width, height, arrayLayers, format, tiling, usage);

  vk_check(vkCreateImage(device_, &imageInfo, NULL, &image));

  VkMemoryRequirements memRequirements;
  VkMemoryAllocateInfo memAllocInfo(vk::initializers::memoryAllocateInfo());

  vkGetImageMemoryRequirements(device_, image, &memRequirements);
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memRequirements.size;
  memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);
  vk_check(vkAllocateMemory(device_, &memAllocInfo, NULL, &memory));
  vk_check(vkBindImageMemory(device_, image, memory, 0));

  return {image, memory};
}

void VulkanDevice::createRenderPass() {
  std::vector<VkAttachmentDescription> attachmentDescriptions = {};
  // Color attachment
  VkAttachmentDescription colorAttachmentDescription = {};
  colorAttachmentDescription.format = colorFormat_;
  colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  attachmentDescriptions.push_back(colorAttachmentDescription);

  VkAttachmentDescription depthAttachmentDescription = {};
  depthAttachmentDescription.format = depthFormat_;
  depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  attachmentDescriptions.push_back(depthAttachmentDescription);

  VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  std::vector<VkSubpassDependency> dependencies;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies.push_back(dependency);

  VkRenderPassCreateInfo renderPassInfo = vk::initializers::renderPassCreateInfo(attachmentDescriptions, dependencies, subpassDescription);

  vk_check(vkCreateRenderPass(device_, &renderPassInfo, NULL, &renderPass_));

  std::vector<VkImageView> attachmentViews;
  attachmentViews.push_back(colorAttachment_.view);
  attachmentViews.push_back(depthAttachment_.view);

  VkFramebufferCreateInfo framebufferCreateInfo = vk::initializers::framebufferCreateInfo(width_, height_, renderPass_, attachmentViews);

  vk_check(vkCreateFramebuffer(device_, &framebufferCreateInfo, NULL, &frameBuffer_));
}

VulkanPipeline VulkanDevice::createSpriteRenderPipeline() {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorPool descriptorPool;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  spdlog::debug("Setting up descriptor pool");

  auto storageBufferCount = 2;
  storageBufferCount += globalVariableSSBOBuffer_.count > 0 ? 1 : 0;
  storageBufferCount += objectVariableSSBOBuffer_.count > 0 ? 1 : 0;

  // Set up descriptor pool
  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
      vk::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
      vk::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
      vk::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount),
  };
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vk::initializers::descriptorPoolCreateInfo(descriptorPoolSizes, 1);
  vk_check(vkCreateDescriptorPool(device_, &descriptorPoolCreateInfo, NULL, &descriptorPool));

  spdlog::debug("Setting up descriptor set layout");

  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
  // Add the sampler to layout bindings for the fragment shader
  setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));

  // Add the uniform environment data
  setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));

  // Add the player info SSBO
  setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));

  // Add the object data SSBO
  setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));

  // Add the global variable SSBO
  if (globalVariableSSBOBuffer_.count > 0) {
    setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));
  }

  // Add the object variable SSBO
  if (objectVariableSSBOBuffer_.count > 0) {
    setLayoutBindings.push_back(vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, setLayoutBindings.size()));
  }

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  vk_check(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayout));

  spdlog::debug("Allocating descriptor sets");
  // Allocate the descriptor set>s
  VkDescriptorSetAllocateInfo allocInfo = vk::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
  vk_check(vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet));

  VkSampler sampler = createTextureSampler();
  spdlog::debug("Updating descriptor sets");

  std::vector<VkWriteDescriptorSet> descriptorWrites{};

  spdlog::debug("Creating image descriptor");
  VkDescriptorImageInfo descriptorImageInfo = vk::initializers::descriptorImageInfo(sampler, spriteImageArrayBuffer_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  descriptorWrites.push_back(vk::initializers::writeImageInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorImageInfo));

  spdlog::debug("Creating environment uniform buffer descriptor");
  VkDescriptorBufferInfo environmentUniformInfo = vk::initializers::descriptorBufferInfo(environmentUniformBuffer_.allocated.buffer, environmentUniformBuffer_.allocatedSize);
  descriptorWrites.push_back(vk::initializers::writeBufferInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &environmentUniformInfo));

  spdlog::debug("Creating player info buffer descriptor for {0} objects", playerInfoSSBOBuffer_.count);
  VkDescriptorBufferInfo playerInfoSSBOInfo = vk::initializers::descriptorBufferInfo(playerInfoSSBOBuffer_.allocated.buffer, playerInfoSSBOBuffer_.allocatedSize);
  descriptorWrites.push_back(vk::initializers::writeBufferInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &playerInfoSSBOInfo));

  spdlog::debug("Creating object data buffer descriptor for {0} objects", objectDataSSBOBuffer_.count);
  VkDescriptorBufferInfo objectDataSSBOInfo = vk::initializers::descriptorBufferInfo(objectDataSSBOBuffer_.allocated.buffer, objectDataSSBOBuffer_.allocatedSize);
  descriptorWrites.push_back(vk::initializers::writeBufferInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &objectDataSSBOInfo));

  if (globalVariableSSBOBuffer_.count > 0) {
    spdlog::debug("Creating global variable buffer descriptor for {0} variables", globalVariableSSBOBuffer_.count);
    VkDescriptorBufferInfo globalVariableSSBOInfo = vk::initializers::descriptorBufferInfo(globalVariableSSBOBuffer_.allocated.buffer, globalVariableSSBOBuffer_.allocatedSize);
    descriptorWrites.push_back(vk::initializers::writeBufferInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &globalVariableSSBOInfo));
  }

  if (objectVariableSSBOBuffer_.count > 0) {
    spdlog::debug("Creating object variable buffer descriptor for {0} variables", objectVariableSSBOBuffer_.count);
    VkDescriptorBufferInfo objectVariableSSBOInfo = vk::initializers::descriptorBufferInfo(objectVariableSSBOBuffer_.allocated.buffer, objectVariableSSBOBuffer_.allocatedSize);
    descriptorWrites.push_back(vk::initializers::writeBufferInfoDescriptorSet(descriptorSet, 0, descriptorWrites.size(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &objectVariableSSBOInfo));
  }

  // Write the descriptor to the device
  vkUpdateDescriptorSets(device_, descriptorWrites.size(), descriptorWrites.data(), 0, NULL);
  spdlog::debug("Updating descriptor sets done");

  spdlog::debug("Creating pipeline layout");

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
  VkPushConstantRange pushConstantRange = vk::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(ObjectPushConstants), 0);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  vk_check(vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, NULL, &pipelineLayout));

  // TODO: not really sure we need a pipeline cache because this is not speed critical
  // VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vk::initializers::pipelineCacheCreateInfo();
  // vk_check(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, NULL, &pipelineCache_));

  // Create pipeline
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vk::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
  VkPipelineRasterizationStateCreateInfo rasterizationState = vk::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
  VkPipelineColorBlendAttachmentState blendAttachmentState = vk::initializers::pipelineColorBlendAttachmentState(VK_TRUE);
  VkPipelineColorBlendStateCreateInfo colorBlendState = vk::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
  VkPipelineDepthStencilStateCreateInfo depthStencilState = vk::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
  VkPipelineViewportStateCreateInfo viewportState = vk::initializers::pipelineViewportStateCreateInfo(1, 1);
  VkPipelineMultisampleStateCreateInfo multisampleState = vk::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  // Dynamic states
  std::vector<VkDynamicState> dynamicStateEnables = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = vk::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

  // Vertex shader
  shaderStages[0].module = loadShader(shaderPath_ + "/triangle-textured.vert.spv", device_);
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";

  // Fragment shader
  shaderStages[1].module = loadShader(shaderPath_ + "/triangle-textured.frag.spv", device_);
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pName = "main";

  // Vertex bindings an attributes
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = TexturedVertex::getBindingDescriptions();
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = TexturedVertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState = vk::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
  vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  // Hook this pipeline to the global render pass
  VkGraphicsPipelineCreateInfo pipelineCreateInfo = vk::initializers::pipelineCreateInfo(pipelineLayout, renderPass_);

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();
  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  spdlog::debug("Creating graphics pipelines");

  vk_check(vkCreateGraphicsPipelines(device_, NULL, 1, &pipelineCreateInfo, NULL, &pipeline));

  return {pipeline, pipelineLayout, descriptorPool, descriptorSetLayout, descriptorSet, shaderStages, sampler};
}

void VulkanDevice::executeCommandBuffer(VkCommandBuffer commandBuffer) {
  VkSubmitInfo submitInfo = vk::initializers::submitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  VkFenceCreateInfo fenceInfo = vk::initializers::fenceCreateInfo();
  VkFence fence;
  vk_check(vkCreateFence(device_, &fenceInfo, NULL, &fence));
  vk_check(vkQueueSubmit(computeQueue_, 1, &submitInfo, fence));
  vk_check(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(device_, fence, NULL);
}

uint8_t* VulkanDevice::renderFrame() {
  executeCommandBuffer(renderContext_.commandBuffer);
  return imageRGBA_;
}

}  // namespace vk