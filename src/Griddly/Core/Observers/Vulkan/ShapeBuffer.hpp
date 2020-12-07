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

struct Vertex {
  glm::vec3 position;

  static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
    return {vk::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)};
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    return {
        vk::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)};
  }
};

struct SpritePushConstants {
  glm::mat4 model;
  glm::vec4 color;
  uint32_t textureIndex;
  float textureMultiplyY = 1.0;
  float textureMultiplyX = 1.0;
  uint32_t isOutline = 0;
  glm::vec4 outlineColor;
};

struct ShapePushConstants {
  glm::mat4 model;
  glm::vec4 color;
  uint32_t isOutline = 0;
  glm::vec4 outlineColor;
};

namespace shapes {

struct Shape {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

// shapes are initialized with colour as black, will copy new colours to the buffers later
const Shape triangle = {
    {{{0.5f, 0.5f, 0.0f}},
     {{-0.5f, 0.5f, 0.0f}},
     {{0.0f, -0.5f, 0.0f}}},
    {{0, 1, 2}}};

const Shape square = {
    {{{0.5f, 0.5f, 0.0f}},
     {{-0.5f, 0.5f, 0.0f}},
     {{-0.5f, -0.5f, 0.0f}},
     {{0.5f, -0.5f, 0.0f}}},
    {0, 1, 2, 2, 3, 0}};

}  // namespace shapes

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