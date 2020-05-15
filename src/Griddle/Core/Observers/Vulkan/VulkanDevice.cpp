#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "VulkanDevice.hpp"

#include "ShapeBuffer.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanInstance.hpp"
#include "VulkanPhysicalDeviceInfo.hpp"
#include "VulkanQueueFamilyIndices.hpp"
#include "VulkanUtil.hpp"

namespace vk {

VulkanDevice::VulkanDevice(std::unique_ptr<vk::VulkanInstance> vulkanInstance, uint32_t width, uint32_t height, uint32_t tileSize)
    : vulkanInstance_(std::move(vulkanInstance)),
      tileSize_(tileSize),
      width_(width),
      height_(height),
      ortho_(glm::ortho(0.0f, (float)width, 0.0f, (float)height, 0.0f, 1.0f)) {
}

VulkanDevice::~VulkanDevice() {
  if (device_ != VK_NULL_HANDLE) {
    // Free all the vertex/index buffers
    for (auto& buffer : shapeBuffers_) {
      vkDestroyBuffer(device_, buffer.second.vertex.buffer, NULL);
      vkFreeMemory(device_, buffer.second.vertex.memory, NULL);
      vkDestroyBuffer(device_, buffer.second.index.buffer, NULL);
      vkFreeMemory(device_, buffer.second.index.memory, NULL);
    }

    // Remove pipelines and shaders
    // vkDestroyPipelineCache(device_, pipelineCache_, NULL);
    // vkDestroyPipelineLayout(device_, pipelineLayout_, NULL);
    // vkDestroyPipeline(device_, pipeline_, NULL);
    // vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, NULL);

    // for (auto& shader : shaderModules_) {
    //   vkDestroyShaderModule(device_, shader, NULL);
    // }

    // Remove frame buffers
    vkDestroyImage(device_, colorAttachment_.image, NULL);
    vkFreeMemory(device_, colorAttachment_.memory, NULL);
    vkDestroyImageView(device_, colorAttachment_.view, NULL);

    vkDestroyImage(device_, depthAttachment_.image, NULL);
    vkFreeMemory(device_, depthAttachment_.memory, NULL);
    vkDestroyImageView(device_, depthAttachment_.view, NULL);

    vkDestroyFramebuffer(device_, frameBuffer_, NULL);

    // Remove render pass
    vkDestroyRenderPass(device_, renderPass_, NULL);

    // Remove the rendering surface
    vkDestroyImage(device_, renderedImage_, NULL);
    vkFreeMemory(device_, renderedImageMemory_, NULL);

    vkDestroyCommandPool(device_, commandPool_, NULL);
    vkDestroyDevice(device_, NULL);

    // Remove local RGB image
    delete[] imageRGB_;
  }
}

void VulkanDevice::initDevice(bool useGPU) {
  spdlog::debug("Initializing Vulkan Device.");
  spdlog::debug("Vulkan Device: width={0} height={1}, tileSize={2}", width_, height_, tileSize_);
  std::vector<VkPhysicalDevice> physicalDevices = getAvailablePhysicalDevices();
  std::vector<VulkanPhysicalDeviceInfo> supportedPhysicalDevices = getSupportedPhysicalDevices(physicalDevices);

  if (supportedPhysicalDevices.size() > 0) {
    //auto physicalDeviceInfo = selectPhysicalDevice(useGPU, supportedPhysicalDevices);
    auto physicalDeviceInfo = &supportedPhysicalDevices[0];

    // This should never be hit if the previous check succeeds, but is here for completeness
    // if (physicalDeviceInfo == supportedPhysicalDevices.end()) {
    //   spdlog::error("Could not select a physical device, isGpu={0}", useGPU);
    //   return;
    // }

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
    vk_check(vkCreateCommandPool(device_, &commandPoolCreateInfo, nullptr, &commandPool_));

    spdlog::debug("Creating colour frame buffer.");
    colorAttachment_ = createColorAttachment();
    spdlog::debug("Creating depth frame buffer.");
    depthAttachment_ = createDepthAttachment();

    spdlog::debug("Creating render pass.");
    createRenderPass();

    spdlog::debug("Allocating offscreen host image data.");
    allocateHostImageData();

  } else {
    spdlog::error("No devices supporting vulkan present for rendering.");
  }
}

void VulkanDevice::initRenderMode(RenderMode mode) {
  switch (mode) {
    case SHAPES:
      spdlog::info("Render mode set to SHAPES. Will only load shape render pipeline.");
      shapeBuffers_ = createShapeBuffers();
      shapeRenderPipeline_ = createShapeRenderPipeline();
      break;
    case SPRITES:
      spdlog::info("Render mode set to SPRITES. Will load both shape and sprite render pipelines.");
      // shapeBuffers_ = createShapeBuffers();
      // shapeRenderPipeline_ = createShapeRenderPipeline();
      spriteShapeBuffer_ = createSpriteShapeBuffer();
      spriteRenderPipeline_ = createSpriteRenderPipeline();
      break;
  }
}

VkCommandBuffer VulkanDevice::beginCommandBuffer() {
  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::initializers::commandBufferAllocateInfo(commandPool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  vk_check(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo cmdBufInfo =
      vk::initializers::commandBufferBeginInfo();

  vk_check(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

  return commandBuffer;
}

void VulkanDevice::endCommandBuffer(VkCommandBuffer& commandBuffer) {
  vk_check(vkEndCommandBuffer(commandBuffer));

  submitCommands(commandBuffer);

  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

VulkanRenderContext VulkanDevice::beginRender() {
  assert(("Cannot begin a render session if already rendering.", !isRendering_));

  vk_check(vkResetCommandPool(device_, commandPool_, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

  isRendering_ = true;

  VulkanRenderContext renderContext = {};

  auto commandBuffer = beginCommandBuffer();

  renderContext.commandBuffer = commandBuffer;

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

  vkCmdBeginRenderPass(renderContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.height = (float)height_;
  viewport.width = (float)width_;
  viewport.minDepth = (float)0.0f;
  viewport.maxDepth = (float)1.0f;
  vkCmdSetViewport(renderContext.commandBuffer, 0, 1, &viewport);

  // Update dynamic scissor state
  VkRect2D scissor = {};
  scissor.extent.width = width_;
  scissor.extent.height = height_;
  vkCmdSetScissor(renderContext.commandBuffer, 0, 1, &scissor);

  return renderContext;
}

ShapeBuffer VulkanDevice::getShapeBuffer(std::string shapeBufferName) {
  auto shapeBufferItem = shapeBuffers_.find(shapeBufferName);
  return shapeBufferItem->second;
}

void VulkanDevice::drawBackgroundTiling(VulkanRenderContext& renderContext, uint32_t arrayLayer) {
  auto commandBuffer = renderContext.commandBuffer;
  auto vertexBuffer = spriteShapeBuffer_.vertex.buffer;
  auto indexBuffer = spriteShapeBuffer_.index.buffer;

  vkCmdBindDescriptorSets(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spriteRenderPipeline_.pipelineLayout, 0, 1, &spriteRenderPipeline_.descriptorSet, 0, nullptr);
  vkCmdBindPipeline(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spriteRenderPipeline_.pipeline);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  glm::vec3 position = {width_/2.0, height_/2.0, -1.0};

  glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), {width_, height_, 1.0f});

  glm::mat4 mvpMatrix = ortho_ * model;

  SpritePushConstants modelColorSprite = {mvpMatrix, glm::vec3(1.0), arrayLayer, (float)height_/tileSize_, (float)width_/tileSize_};
  vkCmdPushConstants(commandBuffer, spriteRenderPipeline_.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SpritePushConstants), &modelColorSprite);
  vkCmdDrawIndexed(commandBuffer, spriteShapeBuffer_.indices, 1, 0, 0, 0);
}

void VulkanDevice::drawShape(VulkanRenderContext& renderContext, ShapeBuffer shapeBuffer, glm::mat4 model, glm::vec3 color) {
  auto commandBuffer = renderContext.commandBuffer;
  auto vertexBuffer = shapeBuffer.vertex.buffer;
  auto indexBuffer = shapeBuffer.index.buffer;

  vkCmdBindPipeline(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shapeRenderPipeline_.pipeline);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  glm::mat4 mvpMatrix = ortho_ * model;

  ShapePushConstants modelAndColor = {mvpMatrix, color};
  vkCmdPushConstants(commandBuffer, shapeRenderPipeline_.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShapePushConstants), &modelAndColor);
  vkCmdDrawIndexed(commandBuffer, shapeBuffer.indices, 1, 0, 0, 0);
}

uint32_t VulkanDevice::getSpriteArrayLayer(std::string spriteName) {
  auto spriteIndexItem = spriteIndices_.find(spriteName);
  return spriteIndexItem->second;
}

void VulkanDevice::drawSprite(VulkanRenderContext& renderContext, uint32_t arrayLayer, glm::mat4 model, glm::vec3 color) {
  auto commandBuffer = renderContext.commandBuffer;
  auto vertexBuffer = spriteShapeBuffer_.vertex.buffer;
  auto indexBuffer = spriteShapeBuffer_.index.buffer;

  vkCmdBindDescriptorSets(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spriteRenderPipeline_.pipelineLayout, 0, 1, &spriteRenderPipeline_.descriptorSet, 0, nullptr);
  vkCmdBindPipeline(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spriteRenderPipeline_.pipeline);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  glm::mat4 mvpMatrix = ortho_ * model;

  SpritePushConstants modelColorSprite = {mvpMatrix, color, arrayLayer};
  vkCmdPushConstants(commandBuffer, spriteRenderPipeline_.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SpritePushConstants), &modelColorSprite);
  vkCmdDrawIndexed(commandBuffer, spriteShapeBuffer_.indices, 1, 0, 0, 0);
}

std::unique_ptr<uint8_t[]> VulkanDevice::endRender(VulkanRenderContext& renderContext, std::vector<VkRect2D> dirtyRectangles = {}) {
  isRendering_ = false;

  auto commandBuffer = renderContext.commandBuffer;

  vkCmdEndRenderPass(commandBuffer);

  endCommandBuffer(commandBuffer);

  vkDeviceWaitIdle(device_);

  copySceneToHostImage(dirtyRectangles);

  return copyHostImage();
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

  endCommandBuffer(commandBuffer);
}

void VulkanDevice::copyImage(VkImage imageSrc, VkImage imageDst, std::vector<VkRect2D> rects) {
  auto commandBuffer = beginCommandBuffer();

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

  endCommandBuffer(commandBuffer);
}

void VulkanDevice::copySceneToHostImage(std::vector<VkRect2D> dirtyRectangles) {
  copyImage(colorAttachment_.image, renderedImage_, dirtyRectangles);

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device_, renderedImage_, &subResource, &subResourceLayout);

  uint8_t* imageRGBA = imageRGBA_ + subResourceLayout.offset;

  for (auto dirtyRect : dirtyRectangles) {
    int bottom = dirtyRect.offset.y;
    int top = bottom + dirtyRect.extent.height;
    int left = dirtyRect.offset.x;
    int right = left + dirtyRect.extent.width;

    for (int32_t y = bottom; y < top; y++) {
      auto dest = (width_ * y + dirtyRect.offset.x) * 3;
      auto src = subResourceLayout.rowPitch * y + dirtyRect.offset.x * 4;
      for (int32_t x = left; x < right; x++) {
        imageRGB_[dest] = imageRGBA[src];
        imageRGB_[dest + 1] = imageRGBA[src + 1];
        imageRGB_[dest + 2] = imageRGBA[src + 2];
        dest += 3;  // RGB
        src += 4;   // RGBA
      }
    }
  }
}

void VulkanDevice::allocateHostImageData() {
  // Create the linear tiled destination image to copy to and to read the memory from

  auto imageBuffer = createImage(width_, height_, 1, colorFormat_, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

  renderedImage_ = imageBuffer.image;
  renderedImageMemory_ = imageBuffer.memory;

  // Map image memory so we can start copying from it
  vkMapMemory(device_, renderedImageMemory_, 0, VK_WHOLE_SIZE, 0, (void**)&imageRGBA_);
  imageRGB_ = new uint8_t[width_ * height_ * 3];
}

std::unique_ptr<uint8_t[]> VulkanDevice::copyHostImage() {
  int bytes = width_ * height_ * 3;
  std::unique_ptr<uint8_t[]> imageRGBCopy(new uint8_t[bytes]);
  memcpy(imageRGBCopy.get(), imageRGB_, bytes);

  return std::move(imageRGBCopy);
}

void VulkanDevice::preloadSprites(std::unordered_map<std::string, SpriteData>& spritesData) {
  auto arrayLayers = spritesData.size();

  spriteImageArrayBuffer_ = createImage(tileSize_, tileSize_, arrayLayers, colorFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageViewCreateInfo spriteImageView = vk::initializers::imageViewCreateInfo(colorFormat_, spriteImageArrayBuffer_.image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, arrayLayers);
  vk_check(vkCreateImageView(device_, &spriteImageView, nullptr, &spriteImageArrayBuffer_.view));

  int layer = 0;
  for (auto& spriteToLoad : spritesData) {
    auto &spriteInfo = spriteToLoad.second;
    auto spriteName = spriteToLoad.first;

    auto spriteSize = spriteInfo.width * spriteInfo.height * spriteInfo.channels;

    auto imageData = spriteInfo.data.get();
    stageToDeviceImage(spriteImageArrayBuffer_.image, imageData, spriteSize, layer);
    spriteIndices_.insert({spriteName, layer});
    layer++;
  }
}

VkSampler VulkanDevice::createTextureSampler() {
  VkSampler textureSampler;
  auto samplerCreateInfo = vk::initializers::samplerCreateInfo();

  spdlog::debug("Creating texture sampler");

  vk_check(vkCreateSampler(device_, &samplerCreateInfo, nullptr, &textureSampler));
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

std::unordered_map<std::string, ShapeBuffer> VulkanDevice::createShapeBuffers() {
  // create triangle buffer

  auto triangleBuffers = createShapeBuffer(shapes::triangle);

  // create square buffer
  auto squareBuffers = createShapeBuffer(shapes::square);

  return {{"triangle", triangleBuffers}, {"square", squareBuffers}};
}

ShapeBuffer VulkanDevice::createShapeBuffer(shapes::Shape shape) {
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

void VulkanDevice::stageToDeviceBuffer(VkBuffer& deviceBuffer, void* data, uint32_t bufferSize) {
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

  endCommandBuffer(commandBuffer);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingMemory, nullptr);

  spdlog::debug("Done!");
}

void VulkanDevice::stageToDeviceImage(VkImage& deviceImage, void* data, uint32_t bufferSize, uint32_t arrayLayer) {
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

  copyBufferToImage(stagingBuffer, deviceImage, {{{0, 0}, {tileSize_, tileSize_}}}, arrayLayer);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingMemory, nullptr);

  spdlog::debug("Done!");
}

void VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data) {
  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo = vk::initializers::bufferCreateInfo(usageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vk_check(vkCreateBuffer(device_, &bufferCreateInfo, nullptr, buffer));

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = vk::initializers::memoryAllocateInfo();
  vkGetBufferMemoryRequirements(device_, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
  vk_check(vkAllocateMemory(device_, &memAlloc, nullptr, memory));

  // Initial memory allocation
  if (data != nullptr) {
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

std::vector<VulkanPhysicalDeviceInfo> VulkanDevice::getSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices) {
  std::vector<VulkanPhysicalDeviceInfo> supportedPhysicalDevices;
  for (auto& physicalDevice : physicalDevices) {
    VulkanPhysicalDeviceInfo physicalDeviceInfo = getPhysicalDeviceInfo(physicalDevice);
    spdlog::info("Device {0}, isGpu {1}, isSupported {2}.", physicalDeviceInfo.deviceName, physicalDeviceInfo.isGpu, physicalDeviceInfo.isSupported);

    if (physicalDeviceInfo.isSupported) {
      supportedPhysicalDevices.push_back(physicalDeviceInfo);
    }
  }

  return supportedPhysicalDevices;
}

std::vector<VkPhysicalDevice> VulkanDevice::getAvailablePhysicalDevices() {
  uint32_t deviceCount = 0;
  vk_check(vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  vk_check(vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, physicalDevices.data()));

  return physicalDevices;
}

VulkanPhysicalDeviceInfo VulkanDevice::getPhysicalDeviceInfo(VkPhysicalDevice& physicalDevice) {
  VulkanQueueFamilyIndices queueFamilyIndices;
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

  auto deviceName = deviceProperties.deviceName;

  spdlog::info("Device found {0}, checking for Vulkan support.", deviceName);

  bool isGpu = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  bool isSupported = hasQueueFamilySupport(physicalDevice, queueFamilyIndices);

  return {
      physicalDevice,
      std::string(deviceName),
      isGpu,
      isSupported,
      queueFamilyIndices};
}

bool VulkanDevice::hasQueueFamilySupport(VkPhysicalDevice& device, VulkanQueueFamilyIndices& queueFamilyIndices) {
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
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
  vk_check(vkCreateImageView(device_, &depthStencilView, nullptr, &depthAttachment.view));

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
  vk_check(vkCreateImageView(device_, &colorImageView, nullptr, &colorAttachment.view));

  return colorAttachment;
}

ImageBuffer VulkanDevice::createImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat& format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
  VkImage image;
  VkDeviceMemory memory;

  VkImageCreateInfo imageInfo = vk::initializers::imageCreateInfo(width, height, arrayLayers, format, tiling, usage);

  vk_check(vkCreateImage(device_, &imageInfo, nullptr, &image));

  VkMemoryRequirements memRequirements;
  VkMemoryAllocateInfo memAllocInfo(vk::initializers::memoryAllocateInfo());

  vkGetImageMemoryRequirements(device_, image, &memRequirements);
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memRequirements.size;
  memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);
  vk_check(vkAllocateMemory(device_, &memAllocInfo, nullptr, &memory));
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

  vk_check(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_));

  std::vector<VkImageView> attachmentViews;
  attachmentViews.push_back(colorAttachment_.view);
  attachmentViews.push_back(depthAttachment_.view);

  VkFramebufferCreateInfo framebufferCreateInfo = vk::initializers::framebufferCreateInfo(width_, height_, renderPass_, attachmentViews);

  vk_check(vkCreateFramebuffer(device_, &framebufferCreateInfo, nullptr, &frameBuffer_));
}

VulkanPipeline VulkanDevice::createShapeRenderPipeline() {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSetLayout descriptorSetLayout;
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  // Create Descriptor bindings. With raw shapes there are not universal buffer objects or samplers so there are no layout bindings
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
  VkDescriptorSetLayoutCreateInfo descriptorLayout = vk::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  vk_check(vkCreateDescriptorSetLayout(device_, &descriptorLayout, nullptr, &descriptorSetLayout));

  // Shape pipeline has push constants for mvp matrix and shape colors
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::initializers::pipelineLayoutCreateInfo(nullptr, 0);
  VkPushConstantRange pushConstantRange = vk::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShapePushConstants), 0);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  vk_check(vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

  // TODO: not really sure we need a pipeline cache because this is not speed critical
  // VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vk::initializers::pipelineCacheCreateInfo();
  // vk_check(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipelineCache_));

  // Create pipeline
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vk::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
  VkPipelineRasterizationStateCreateInfo rasterizationState = vk::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
  VkPipelineColorBlendAttachmentState blendAttachmentState = vk::initializers::pipelineColorBlendAttachmentState(VK_FALSE);
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
  shaderStages[0].module = loadShader("resources/shaders/triangle.vert.spv", device_);
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";

  // Fragment shader
  shaderStages[1].module = loadShader("resources/shaders/triangle.frag.spv", device_);
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pName = "main";

  // Vertex bindings an attributes
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = Vertex::getBindingDescriptions();
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = Vertex::getAttributeDescriptions();

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

  vk_check(vkCreateGraphicsPipelines(device_, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));

  return {pipeline, pipelineLayout, descriptorSetLayout, nullptr, shaderStages};
}

VulkanPipeline VulkanDevice::createSpriteRenderPipeline() {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorPool descriptorPool;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  spdlog::debug("Setting up descriptor set layout");
  // Add the sampler to layout bindings for the fragment shader
  VkDescriptorSetLayoutBinding samplerLayoutBinding = vk::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  vk_check(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

  spdlog::debug("Creating pipeline layout");

  // Shape pipeline has push constants for mvp matrix and shape colors
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
  VkPushConstantRange pushConstantRange = vk::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(SpritePushConstants), 0);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  vk_check(vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

  // TODO: not really sure we need a pipeline cache because this is not speed critical
  // VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vk::initializers::pipelineCacheCreateInfo();
  // vk_check(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipelineCache_));

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
  shaderStages[0].module = loadShader("resources/shaders/triangle-textured.vert.spv", device_);
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";

  // Fragment shader
  shaderStages[1].module = loadShader("resources/shaders/triangle-textured.frag.spv", device_);
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

  vk_check(vkCreateGraphicsPipelines(device_, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));

  spdlog::debug("Setting up descriptor pool");

  // Set up descriptor pool
  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {vk::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vk::initializers::descriptorPoolCreateInfo(descriptorPoolSizes, 1);
  vk_check(vkCreateDescriptorPool(device_, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
  spdlog::debug("Allocating descriptor sets");
  // Allocate the descriptor sets
  VkDescriptorSetAllocateInfo allocInfo = vk::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
  vk_check(vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet));

  VkSampler sampler = createTextureSampler();
  spdlog::debug("Updating descriptor sets");
  VkDescriptorImageInfo descriptorImageInfo = vk::initializers::descriptorImageInfo(sampler, spriteImageArrayBuffer_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Write the image info descriptor to the device
  std::array<VkWriteDescriptorSet, 1> descriptorWrites;
  VkWriteDescriptorSet descriptorWrite;
  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].pNext = nullptr;
  descriptorWrites[0].dstSet = descriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pImageInfo = &descriptorImageInfo;
  descriptorWrites[0].pBufferInfo = nullptr;
  descriptorWrites[0].pTexelBufferView = nullptr;

  vkUpdateDescriptorSets(device_, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  spdlog::debug("Updating descriptor sets done");

  return {pipeline, pipelineLayout, descriptorSetLayout, descriptorSet, shaderStages};
}

void VulkanDevice::submitCommands(VkCommandBuffer cmdBuffer) {
  VkSubmitInfo submitInfo = vk::initializers::submitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;
  VkFenceCreateInfo fenceInfo = vk::initializers::fenceCreateInfo();
  VkFence fence;
  vk_check(vkCreateFence(device_, &fenceInfo, nullptr, &fence));
  vk_check(vkQueueSubmit(computeQueue_, 1, &submitInfo, fence));
  vk_check(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(device_, fence, nullptr);
}

}  // namespace vk