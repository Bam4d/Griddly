#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "VulkanInitializers.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vk {

struct TexturedVertex {
  glm::vec3 position;
  glm::vec2 texCoord;

  static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
    return {vk::initializers::vertexInputBindingDescription(0, sizeof(TexturedVertex), VK_VERTEX_INPUT_RATE_VERTEX)};
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    return {
        vk::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TexturedVertex, position)),
        vk::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TexturedVertex, texCoord))};
  }
};

struct ObjectPushConstants {
  uint32_t objectIndex;
};

namespace sprite {
struct TexturedShape {
  std::vector<TexturedVertex> vertices;
  std::vector<uint32_t> indices;
};

const TexturedShape squareSprite = {
    {{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
     {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},
     {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
     {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}},
    {0, 1, 2, 2, 3, 0}};
}  // namespace sprite

}  // namespace vk