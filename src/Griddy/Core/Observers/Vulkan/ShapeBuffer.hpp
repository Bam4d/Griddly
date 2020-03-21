#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "VulkanInitializers.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vk {

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

struct ShapePushConstants {
  glm::mat4 model;
  glm::vec3 color;
};

namespace shapes {

struct Shape {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

// shapes are initialized with colour as black, will copy new colours to the buffers later
Shape triangle = {
    {{{0.5f, 0.5f, 0.0f}},
     {{-0.5f, 0.5f, 0.0f}},
     {{0.0f, -0.5f, 0.0f}}},
    {{0, 1, 2}}};

Shape square = {
    {{{0.5f, 0.5f, 0.0f}},
     {{-0.5f, 0.5f, 0.0f}},
     {{-0.5f, -0.5f, 0.0f}},
     {{0.5f, -0.5f, 0.0f}}},
    {0, 1, 2, 2, 3, 0}};

}  // namespace shapes

}  // namespace vk