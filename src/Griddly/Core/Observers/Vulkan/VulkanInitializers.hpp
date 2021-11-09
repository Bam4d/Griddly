#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanConfiguration.hpp"

namespace vk {
namespace initializers {

inline VkApplicationInfo applicationInfo(VulkanConfiguration& config) {
  VkApplicationInfo applicationInfo{};
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pApplicationName = config.applicationName;
  applicationInfo.pEngineName = config.engineName;
  applicationInfo.applicationVersion = config.applicationVersion;
  applicationInfo.engineVersion = config.engineVersion;
  applicationInfo.apiVersion = config.apiVersion;
  return applicationInfo;
}

inline VkInstanceCreateInfo instanceCreateInfo(VkApplicationInfo& applicationInfo, std::vector<const char*>& layers, std::vector<const char*>& extensions) {
  VkInstanceCreateInfo createInstanceInfo{};
  createInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInstanceInfo.pApplicationInfo = &applicationInfo;
  createInstanceInfo.enabledExtensionCount = extensions.size();
  createInstanceInfo.ppEnabledExtensionNames = extensions.data();
  createInstanceInfo.enabledLayerCount = layers.size();
  createInstanceInfo.ppEnabledLayerNames = layers.data();
  return createInstanceInfo;
}

inline VkDeviceQueueCreateInfo deviceQueueCreateInfo(const uint32_t& queueFamilyIndex, const float& priority) {
  VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
  deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
  deviceQueueCreateInfo.queueCount = 1;
  deviceQueueCreateInfo.pQueuePriorities = &priority;
  return deviceQueueCreateInfo;
}

inline VkDeviceCreateInfo deviceCreateInfo(VkDeviceQueueCreateInfo& deviceQueueCreateInfo) {
  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
  return deviceCreateInfo;
}

inline VkMemoryAllocateInfo memoryAllocateInfo() {
  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  return memAllocInfo;
}

inline VkMappedMemoryRange mappedMemoryRange() {
  VkMappedMemoryRange mappedMemoryRange{};
  mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  return mappedMemoryRange;
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
    VkCommandPool commandPool,
    VkCommandBufferLevel level,
    uint32_t bufferCount) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = level;
  commandBufferAllocateInfo.commandBufferCount = bufferCount;
  return commandBufferAllocateInfo;
}

inline VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex) {
  VkCommandPoolCreateInfo cmdPoolCreateInfo{};
  cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
  cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  return cmdPoolCreateInfo;
}

inline VkCommandBufferBeginInfo commandBufferBeginInfo() {
  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  return cmdBufferBeginInfo;
}

inline VkCommandBufferInheritanceInfo commandBufferInheritanceInfo() {
  VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
  cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  return cmdBufferInheritanceInfo;
}

inline VkRenderPassBeginInfo renderPassBeginInfo() {
  VkRenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  return renderPassBeginInfo;
}

inline VkRenderPassCreateInfo renderPassCreateInfo(std::vector<VkAttachmentDescription>& attachments, std::vector<VkSubpassDependency>& dependencies, VkSubpassDescription& subpassDescription) {
  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassCreateInfo.pAttachments = attachments.data();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpassDescription;
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassCreateInfo.pDependencies = dependencies.data();
  return renderPassCreateInfo;
}

/** @brief Initialize an image memory barrier with no image transfer ownership */
inline VkImageMemoryBarrier imageMemoryBarrier() {
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  return imageMemoryBarrier;
}

/** @brief Initialize a buffer memory barrier with no image transfer ownership */
inline VkBufferMemoryBarrier bufferMemoryBarrier() {
  VkBufferMemoryBarrier bufferMemoryBarrier{};
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  return bufferMemoryBarrier;
}

inline VkMemoryBarrier memoryBarrier() {
  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  return memoryBarrier;
}

inline VkImageCreateInfo imageCreateInfo(uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat& format, VkImageTiling& tiling, VkImageUsageFlags usageFlags, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED) {
  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.extent.width = width;
  imageCreateInfo.extent.height = height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.initialLayout = initialLayout;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = arrayLayers;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = tiling;
  imageCreateInfo.usage = usageFlags;
  return imageCreateInfo;
}

inline VkSamplerCreateInfo samplerCreateInfo() {
  VkSamplerCreateInfo samplerCreateInfo{};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.compareEnable = VK_FALSE;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerCreateInfo.maxAnisotropy = 1.0f;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = 0.0f;
  return samplerCreateInfo;
}

inline VkImageViewCreateInfo imageViewCreateInfo(VkFormat& colorFormat, VkImage& image, VkImageViewType viewType, VkImageAspectFlags aspectMask, uint32_t layerCount = 1) {
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = viewType;
  imageViewCreateInfo.format = colorFormat;
  imageViewCreateInfo.subresourceRange = {aspectMask, 0, 1, 0, layerCount};
  imageViewCreateInfo.image = image;
  return imageViewCreateInfo;
}

inline VkFramebufferCreateInfo framebufferCreateInfo(uint32_t width, uint32_t height, VkRenderPass& renderPass, std::vector<VkImageView>& attachments) {
  VkFramebufferCreateInfo framebufferCreateInfo{};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = renderPass;
  framebufferCreateInfo.attachmentCount = attachments.size();
  framebufferCreateInfo.pAttachments = attachments.data();
  framebufferCreateInfo.width = width;
  framebufferCreateInfo.height = height;
  framebufferCreateInfo.layers = 1;
  return framebufferCreateInfo;
}

inline VkSemaphoreCreateInfo semaphoreCreateInfo() {
  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  return semaphoreCreateInfo;
}

inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = flags;
  return fenceCreateInfo;
}

inline VkEventCreateInfo eventCreateInfo() {
  VkEventCreateInfo eventCreateInfo{};
  eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
  return eventCreateInfo;
}

inline VkSubmitInfo submitInfo() {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  return submitInfo;
}

inline VkViewport viewport(
    float width,
    float height,
    float minDepth,
    float maxDepth) {
  VkViewport viewport{};
  viewport.width = width;
  viewport.height = height;
  viewport.minDepth = minDepth;
  viewport.maxDepth = maxDepth;
  return viewport;
}

inline VkRect2D rect2D(
    int32_t width,
    int32_t height,
    int32_t offsetX,
    int32_t offsetY) {
  VkRect2D rect2D{};
  rect2D.extent.width = width;
  rect2D.extent.height = height;
  rect2D.offset.x = offsetX;
  rect2D.offset.y = offsetY;
  return rect2D;
}

inline VkBufferCreateInfo bufferCreateInfo() {
  VkBufferCreateInfo bufCreateInfo{};
  bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  return bufCreateInfo;
}

inline VkBufferCreateInfo bufferCreateInfo(
    VkBufferUsageFlags usage,
    VkDeviceSize size) {
  VkBufferCreateInfo bufCreateInfo{};
  bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufCreateInfo.usage = usage;
  bufCreateInfo.size = size;
  return bufCreateInfo;
}

inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
    uint32_t poolSizeCount,
    VkDescriptorPoolSize* pPoolSizes,
    uint32_t maxSets) {
  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.poolSizeCount = poolSizeCount;
  descriptorPoolInfo.pPoolSizes = pPoolSizes;
  descriptorPoolInfo.maxSets = maxSets;
  return descriptorPoolInfo;
}

inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets) {
  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  descriptorPoolInfo.maxSets = maxSets;
  return descriptorPoolInfo;
}

inline VkDescriptorPoolSize descriptorPoolSize(
    VkDescriptorType type,
    uint32_t descriptorCount) {
  VkDescriptorPoolSize descriptorPoolSize{};
  descriptorPoolSize.type = type;
  descriptorPoolSize.descriptorCount = descriptorCount;
  return descriptorPoolSize;
}

inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
    VkDescriptorType type,
    VkShaderStageFlags stageFlags,
    uint32_t binding,
    uint32_t descriptorCount = 1) {
  VkDescriptorSetLayoutBinding setLayoutBinding{};
  setLayoutBinding.descriptorType = type;
  setLayoutBinding.stageFlags = stageFlags;
  setLayoutBinding.binding = binding;
  setLayoutBinding.descriptorCount = descriptorCount;
  setLayoutBinding.pImmutableSamplers = NULL;
  return setLayoutBinding;
}

inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    const VkDescriptorSetLayoutBinding* pBindings,
    uint32_t bindingCount) {
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.pBindings = pBindings;
  descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
  
  return descriptorSetLayoutCreateInfo;
}

inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  return descriptorSetLayoutCreateInfo;
}

inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    const VkDescriptorSetLayout* pSetLayouts,
    uint32_t setLayoutCount = 1) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
  pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
  return pipelineLayoutCreateInfo;
}

inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    uint32_t setLayoutCount = 1) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
  return pipelineLayoutCreateInfo;
}

inline VkPipelineCacheCreateInfo pipelineCacheCreateInfo() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  return pipelineCacheCreateInfo;
}

inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
    VkDescriptorPool descriptorPool,
    const VkDescriptorSetLayout* pSetLayouts,
    uint32_t descriptorSetCount) {
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
  descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
  return descriptorSetAllocateInfo;
}

inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout) {
  VkDescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.sampler = sampler;
  descriptorImageInfo.imageView = imageView;
  descriptorImageInfo.imageLayout = imageLayout;
  return descriptorImageInfo;
}

template <class T>
VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, uint32_t maxObjects) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(T) * maxObjects;
  return bufferInfo;
}

inline VkWriteDescriptorSet writeBufferInfoDescriptorSet(
    VkDescriptorSet dstSet,
    uint32_t dstArrayElement,
    uint32_t binding,
    VkDescriptorType type,
    VkDescriptorBufferInfo* bufferInfo,
    uint32_t descriptorCount = 1) {
  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = dstSet;
  writeDescriptorSet.dstArrayElement = dstArrayElement;
  writeDescriptorSet.dstBinding = binding;
  writeDescriptorSet.descriptorType = type;
  writeDescriptorSet.pBufferInfo = bufferInfo;
  writeDescriptorSet.descriptorCount = descriptorCount;
  return writeDescriptorSet;
}

inline VkWriteDescriptorSet writeImageInfoDescriptorSet(
    VkDescriptorSet dstSet,
    uint32_t dstArrayElement,
    uint32_t binding,
    VkDescriptorType type,
    VkDescriptorImageInfo* imageInfo,
    uint32_t descriptorCount = 1) {
  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = dstSet;
  writeDescriptorSet.dstArrayElement = dstArrayElement;
  writeDescriptorSet.dstBinding = binding;
  writeDescriptorSet.descriptorType = type;
  writeDescriptorSet.pImageInfo = imageInfo;
  writeDescriptorSet.descriptorCount = descriptorCount;
  return writeDescriptorSet;
}

inline VkVertexInputBindingDescription vertexInputBindingDescription(
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate) {
  VkVertexInputBindingDescription vInputBindDescription{};
  vInputBindDescription.binding = binding;
  vInputBindDescription.stride = stride;
  vInputBindDescription.inputRate = inputRate;
  return vInputBindDescription;
}

inline VkVertexInputAttributeDescription vertexInputAttributeDescription(
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset) {
  VkVertexInputAttributeDescription vInputAttribDescription{};
  vInputAttribDescription.location = location;
  vInputAttribDescription.binding = binding;
  vInputAttribDescription.format = format;
  vInputAttribDescription.offset = offset;
  return vInputAttribDescription;
}

inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo() {
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
  pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  return pipelineVertexInputStateCreateInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
    VkPrimitiveTopology topology,
    VkPipelineInputAssemblyStateCreateFlags flags,
    VkBool32 primitiveRestartEnable) {
  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
  pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  pipelineInputAssemblyStateCreateInfo.topology = topology;
  pipelineInputAssemblyStateCreateInfo.flags = flags;
  pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
  return pipelineInputAssemblyStateCreateInfo;
}

inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
    VkPolygonMode polygonMode,
    VkCullModeFlags cullMode,
    VkFrontFace frontFace,
    VkPipelineRasterizationStateCreateFlags flags = 0) {
  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
  pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
  pipelineRasterizationStateCreateInfo.cullMode = cullMode;
  pipelineRasterizationStateCreateInfo.frontFace = frontFace;
  pipelineRasterizationStateCreateInfo.flags = flags;
  pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
  pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
  return pipelineRasterizationStateCreateInfo;
}

inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(VkBool32 blendEnable) {
  VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
  pipelineColorBlendAttachmentState.blendEnable = blendEnable;
  pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  return pipelineColorBlendAttachmentState;
}

inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
    uint32_t attachmentCount,
    const VkPipelineColorBlendAttachmentState* pAttachments) {
  VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
  pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
  pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
  pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
  pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
  pipelineColorBlendStateCreateInfo.blendConstants[0] = 1.0f;
  pipelineColorBlendStateCreateInfo.blendConstants[1] = 1.0f;
  pipelineColorBlendStateCreateInfo.blendConstants[2] = 1.0f;
  pipelineColorBlendStateCreateInfo.blendConstants[3] = 1.0f;
  return pipelineColorBlendStateCreateInfo;
}

inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
    VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp) {
  VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
  pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
  pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
  pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
  return pipelineDepthStencilStateCreateInfo;
}

inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
    uint32_t viewportCount,
    uint32_t scissorCount,
    VkPipelineViewportStateCreateFlags flags = 0) {
  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
  pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pipelineViewportStateCreateInfo.viewportCount = viewportCount;
  pipelineViewportStateCreateInfo.scissorCount = scissorCount;
  pipelineViewportStateCreateInfo.flags = flags;
  return pipelineViewportStateCreateInfo;
}

inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples,
    VkPipelineMultisampleStateCreateFlags flags = 0) {
  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
  pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
  pipelineMultisampleStateCreateInfo.flags = flags;
  return pipelineMultisampleStateCreateInfo;
}

inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
    const VkDynamicState* pDynamicStates,
    uint32_t dynamicStateCount,
    VkPipelineDynamicStateCreateFlags flags = 0) {
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
  pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
  pipelineDynamicStateCreateInfo.flags = flags;
  return pipelineDynamicStateCreateInfo;
}

inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
    const std::vector<VkDynamicState>& pDynamicStates,
    VkPipelineDynamicStateCreateFlags flags = 0) {
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
  pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
  pipelineDynamicStateCreateInfo.flags = flags;
  return pipelineDynamicStateCreateInfo;
}

inline VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints) {
  VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
  pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
  return pipelineTessellationStateCreateInfo;
}

inline VkGraphicsPipelineCreateInfo pipelineCreateInfo(
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    VkPipelineCreateFlags flags = 0) {
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = layout;
  pipelineCreateInfo.renderPass = renderPass;
  pipelineCreateInfo.flags = flags;
  pipelineCreateInfo.basePipelineIndex = -1;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  return pipelineCreateInfo;
}

inline VkGraphicsPipelineCreateInfo pipelineCreateInfo() {
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.basePipelineIndex = -1;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  return pipelineCreateInfo;
}

inline VkComputePipelineCreateInfo computePipelineCreateInfo(
    VkPipelineLayout layout,
    VkPipelineCreateFlags flags = 0) {
  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = layout;
  computePipelineCreateInfo.flags = flags;
  return computePipelineCreateInfo;
}

inline VkPushConstantRange pushConstantRange(
    VkShaderStageFlags stageFlags,
    uint32_t size,
    uint32_t offset) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = stageFlags;
  pushConstantRange.offset = offset;
  pushConstantRange.size = size;
  return pushConstantRange;
}

inline VkBindSparseInfo bindSparseInfo() {
  VkBindSparseInfo bindSparseInfo{};
  bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
  return bindSparseInfo;
}

/** @brief Initialize a map entry for a shader specialization constant */
inline VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size) {
  VkSpecializationMapEntry specializationMapEntry{};
  specializationMapEntry.constantID = constantID;
  specializationMapEntry.offset = offset;
  specializationMapEntry.size = size;
  return specializationMapEntry;
}

/** @brief Initialize a specialization constant info structure to pass to a shader stage */
inline VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data) {
  VkSpecializationInfo specializationInfo{};
  specializationInfo.mapEntryCount = mapEntryCount;
  specializationInfo.pMapEntries = mapEntries;
  specializationInfo.dataSize = dataSize;
  specializationInfo.pData = data;
  return specializationInfo;
}
}  // namespace initializers
}  // namespace vk