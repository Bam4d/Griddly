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
    vkDestroyPipelineCache(device_, pipelineCache_, NULL);
    vkDestroyPipelineLayout(device_, pipelineLayout_, NULL);
    vkDestroyPipeline(device_, pipeline_, NULL);
    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, NULL);

    for (auto& shader : shaderModules_) {
      vkDestroyShaderModule(device_, shader, NULL);
    }

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
  }
}

void VulkanDevice::initDevice(bool useGPU) {
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

    auto physicalDevice = physicalDeviceInfo->physicalDevice;

    vk_check(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device_));
    vkGetDeviceQueue(device_, computeQueueFamilyIndex, 0, &computeQueue_);

    auto commandPoolCreateInfo = vk::initializers::commandPoolCreateInfo(computeQueueFamilyIndex);
    vk_check(vkCreateCommandPool(device_, &commandPoolCreateInfo, nullptr, &commandPool_));

    // Command buffer for copy commands (reused)
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::initializers::commandBufferAllocateInfo(commandPool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    vk_check(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &copyCmd_));

    shapeBuffers_ = createShapeBuffers(physicalDevice);
    colorAttachment_ = createColorAttachment(physicalDevice);
    depthAttachment_ = createDepthAttachment(physicalDevice);
    createRenderPass();
    createGraphicsPipeline();
    allocateHostImageData(physicalDevice);

  } else {
    spdlog::error("No devices supporting vulkan present for rendering.");
  }
}

VulkanRenderContext VulkanDevice::beginRender() {
  assert(("Cannot begin a render session if already rendering.", !isRendering_));

  isRendering_ = true;

  VulkanRenderContext renderContext = {};

  VkCommandBufferAllocateInfo cmdBufAllocateInfo = vk::initializers::commandBufferAllocateInfo(commandPool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  vk_check(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &renderContext.commandBuffer));

  VkCommandBufferBeginInfo cmdBufInfo =
      vk::initializers::commandBufferBeginInfo();

  vk_check(vkBeginCommandBuffer(renderContext.commandBuffer, &cmdBufInfo));

  VkClearValue clearValues[1];
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

  vkCmdBindPipeline(renderContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

  return renderContext;
}

void VulkanDevice::drawShape(VulkanRenderContext& renderContext, ShapeBuffer shapeBuffer, glm::mat4 model, glm::vec3 color) {
  auto commandBuffer = renderContext.commandBuffer;
  auto vertexBuffer = shapeBuffer.vertex.buffer;
  auto indexBuffer = shapeBuffer.index.buffer;

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  glm::mat4 mvpMatrix = ortho_ * model;

  ShapePushConstants modelAndColor = {mvpMatrix, color};
  vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShapePushConstants), &modelAndColor);
  vkCmdDrawIndexed(commandBuffer, shapeBuffer.indices, 1, 0, 0, 0);
}

ShapeBuffer VulkanDevice::getShapeBuffer(std::string shapeBufferName) {
  auto shapeBuffer = shapeBuffers_.find(shapeBufferName);
  return shapeBuffer->second;
}

std::unique_ptr<uint8_t[]> VulkanDevice::endRender(VulkanRenderContext& renderContext) {
  isRendering_ = false;

  auto commandBuffer = renderContext.commandBuffer;

  vkCmdEndRenderPass(commandBuffer);

  vk_check(vkEndCommandBuffer(commandBuffer));

  submitCommands(commandBuffer);

  vkDeviceWaitIdle(device_);

  return copySceneToHostImage();
}

std::unique_ptr<uint8_t[]> VulkanDevice::copySceneToHostImage() {
  VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::commandBufferBeginInfo();
  vk_check(vkBeginCommandBuffer(copyCmd_, &cmdBufInfo));

  // Transition destination image to transfer destination layout
  vk::insertImageMemoryBarrier(
      copyCmd_,
      renderedImage_,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

  // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = width_;
  imageCopyRegion.extent.height = height_;
  imageCopyRegion.extent.depth = 1;

  vkCmdCopyImage(
      copyCmd_,
      colorAttachment_.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      renderedImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageCopyRegion);

  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  vk::insertImageMemoryBarrier(
      copyCmd_,
      renderedImage_,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

  vk_check(vkEndCommandBuffer(copyCmd_));

  submitCommands(copyCmd_);

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device_, renderedImage_, &subResource, &subResourceLayout);

  uint8_t* imageRGBA = imageRGBA_ + subResourceLayout.offset;

  std::unique_ptr<uint8_t[]> imageRGB(new uint8_t[width_ * height_ * 3]);

  unsigned int dest = 0;
  // ppm binary pixel data
  // TODO: this can be optimized
  for (int32_t y = 0; y < height_; y++) {
    unsigned int* row = (unsigned int*)imageRGBA;
    for (int32_t x = 0; x < width_; x++) {
      imageRGB[dest] = *row;
      // imageRGB[dest++] = *((char*)row + 1);
      // imageRGB[dest++] = *((char*)row + 2);
      row++;
      dest+=3;
    }
    imageRGBA += subResourceLayout.rowPitch;
  }

  return imageRGB;
}

void VulkanDevice::allocateHostImageData(VkPhysicalDevice& physicalDevice) {
  // Create the linear tiled destination image to copy to and to read the memory from
  VkImageCreateInfo imgCreateInfo = vk::initializers::imageCreateInfo(width_, height_, colorFormat_, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;

  vk_check(vkCreateImage(device_, &imgCreateInfo, nullptr, &renderedImage_));
  // Create memory to back up the image
  VkMemoryRequirements memRequirements;
  VkMemoryAllocateInfo memAllocInfo(vk::initializers::memoryAllocateInfo());

  vkGetImageMemoryRequirements(device_, renderedImage_, &memRequirements);
  memAllocInfo.allocationSize = memRequirements.size;
  // Memory must be host visible to copy from
  memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vk_check(vkAllocateMemory(device_, &memAllocInfo, nullptr, &renderedImageMemory_));
  vk_check(vkBindImageMemory(device_, renderedImage_, renderedImageMemory_, 0));

  // Map image memory so we can start copying from it
  vkMapMemory(device_, renderedImageMemory_, 0, VK_WHOLE_SIZE, 0, (void**)&imageRGBA_);
}

std::unordered_map<std::string, ShapeBuffer> VulkanDevice::createShapeBuffers(VkPhysicalDevice& physicalDevice) {
  // create triangle buffer

  auto triangleBuffers = createShapeBuffer(physicalDevice, vk::shapes::triangle);

  // create square buffer
  auto squareBuffers = createShapeBuffer(physicalDevice, vk::shapes::square);

  return {{"triangle", triangleBuffers}, {"square", squareBuffers}};
}

ShapeBuffer VulkanDevice::createShapeBuffer(VkPhysicalDevice& physicalDevice, shapes::Shape shape) {
  // Vertex Buffers
  auto vertexBuffer = createVertexBuffers(physicalDevice, shape.vertices);

  // Index Buffers
  auto indexBuffer = createIndexBuffers(physicalDevice, shape.indices);

  return {shape.indices.size(), vertexBuffer, indexBuffer};
}

BufferAndMemory VulkanDevice::createVertexBuffers(VkPhysicalDevice& physicalDevice, std::vector<Vertex>& vertices) {
  const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexMemory;

  spdlog::debug("Creating vertex buffer.");
  createBuffer(
      physicalDevice,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &vertexBuffer,
      &vertexMemory,
      vertexBufferSize);

  stageBuffersToDevice(physicalDevice, vertexBuffer, vertices.data(), vertexBufferSize);

  return {vertexBuffer, vertexMemory};
}

BufferAndMemory VulkanDevice::createIndexBuffers(VkPhysicalDevice& physicalDevice, std::vector<uint32_t>& indices) {
  const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

  VkBuffer indexBuffer;
  VkDeviceMemory indexMemory;

  spdlog::debug("Creating index buffer.");
  createBuffer(
      physicalDevice,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &indexBuffer,
      &indexMemory,
      indexBufferSize);

  stageBuffersToDevice(physicalDevice, indexBuffer, indices.data(), indexBufferSize);

  return {indexBuffer, indexMemory};
}

void VulkanDevice::stageBuffersToDevice(VkPhysicalDevice& physicalDevice, VkBuffer& deviceBuffer, void* data, uint32_t bufferSize) {
  VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::commandBufferBeginInfo();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  spdlog::debug("Creating staging memory buffers to transfer {0} bytes.", bufferSize);
  createBuffer(
      physicalDevice,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory,
      bufferSize,
      data);

  vk_check(vkBeginCommandBuffer(copyCmd_, &cmdBufInfo));
  VkBufferCopy copyRegion = {};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(copyCmd_, stagingBuffer, deviceBuffer, 1, &copyRegion);
  vk_check(vkEndCommandBuffer(copyCmd_));

  spdlog::debug("Copying to device...");
  submitCommands(copyCmd_);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingMemory, nullptr);

  spdlog::debug("Done!");
}

void VulkanDevice::createBuffer(VkPhysicalDevice& physicalDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, VkDeviceSize size, void* data) {
  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo = vk::initializers::bufferCreateInfo(usageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vk_check(vkCreateBuffer(device_, &bufferCreateInfo, nullptr, buffer));

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = vk::initializers::memoryAllocateInfo();
  vkGetBufferMemoryRequirements(device_, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryTypeBits, memoryPropertyFlags);
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

uint32_t VulkanDevice::findMemoryTypeIndex(VkPhysicalDevice& physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
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

FrameBufferAttachment VulkanDevice::createDepthAttachment(VkPhysicalDevice& physicalDevice) {
  FrameBufferAttachment depthAttachment;

  getSupportedDepthFormat(physicalDevice, &depthFormat_);

  VkImageCreateInfo imageCreateInfo = vk::initializers::imageCreateInfo(width_, height_, depthFormat_, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkMemoryAllocateInfo memAlloc = vk::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  vk_check(vkCreateImage(device_, &imageCreateInfo, nullptr, &depthAttachment.image));
  vkGetImageMemoryRequirements(device_, depthAttachment.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vk_check(vkAllocateMemory(device_, &memAlloc, nullptr, &depthAttachment.memory));
  vk_check(vkBindImageMemory(device_, depthAttachment.image, depthAttachment.memory, 0));

  VkImageViewCreateInfo depthStencilView = vk::initializers::imageViewCreateInfo(depthFormat_, depthAttachment.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
  vk_check(vkCreateImageView(device_, &depthStencilView, nullptr, &depthAttachment.view));

  return depthAttachment;
}

FrameBufferAttachment VulkanDevice::createColorAttachment(VkPhysicalDevice& physicalDevice) {
  FrameBufferAttachment colorAttachment;

  VkImageCreateInfo imageCreateInfo = vk::initializers::imageCreateInfo(width_, height_, colorFormat_, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  VkMemoryAllocateInfo memAlloc = vk::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  vk_check(vkCreateImage(device_, &imageCreateInfo, nullptr, &colorAttachment.image));
  vkGetImageMemoryRequirements(device_, colorAttachment.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vk_check(vkAllocateMemory(device_, &memAlloc, nullptr, &colorAttachment.memory));
  vk_check(vkBindImageMemory(device_, colorAttachment.image, colorAttachment.memory, 0));

  VkImageViewCreateInfo colorImageView = vk::initializers::imageViewCreateInfo(colorFormat_, colorAttachment.image, VK_IMAGE_ASPECT_COLOR_BIT);
  vk_check(vkCreateImageView(device_, &colorImageView, nullptr, &colorAttachment.view));

  return colorAttachment;
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

void VulkanDevice::createGraphicsPipeline() {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
  VkDescriptorSetLayoutCreateInfo descriptorLayout = vk::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  vk_check(vkCreateDescriptorSetLayout(device_, &descriptorLayout, nullptr, &descriptorSetLayout_));

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::initializers::pipelineLayoutCreateInfo(nullptr, 0);

  VkPushConstantRange pushConstantRange = vk::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShapePushConstants), 0);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  vk_check(vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout_));

  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vk::initializers::pipelineCacheCreateInfo();
  vk_check(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipelineCache_));

  // Create pipeline
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vk::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
  VkPipelineRasterizationStateCreateInfo rasterizationState = vk::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
  VkPipelineColorBlendAttachmentState blendAttachmentState = vk::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
  VkPipelineColorBlendStateCreateInfo colorBlendState = vk::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
  VkPipelineDepthStencilStateCreateInfo depthStencilState = vk::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
  VkPipelineViewportStateCreateInfo viewportState = vk::initializers::pipelineViewportStateCreateInfo(1, 1);
  VkPipelineMultisampleStateCreateInfo multisampleState = vk::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkDynamicState> dynamicStateEnables = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = vk::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
  VkGraphicsPipelineCreateInfo pipelineCreateInfo = vk::initializers::pipelineCreateInfo(pipelineLayout_, renderPass_);

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // Vertex bindings an attributes
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = Vertex::getBindingDescriptions();
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState = vk::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
  vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  shaderStages[0].module = loadShader("resources/shaders/triangle.vert.spv", device_);
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";

  shaderStages[1].module = loadShader("resources/shaders/triangle.frag.spv", device_);
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pName = "main";

  shaderModules_ = {shaderStages[0].module, shaderStages[1].module};
  vk_check(vkCreateGraphicsPipelines(device_, pipelineCache_, 1, &pipelineCreateInfo, nullptr, &pipeline_));
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