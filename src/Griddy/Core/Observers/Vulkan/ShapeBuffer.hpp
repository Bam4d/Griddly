#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <array>

#include "VulkanInitializers.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vk {

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    return vk::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {
        vk::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
        vk::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
    };

    return attributeDescriptions;
  }

};

namespace shapes {

struct Shape {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

// shapes are initialized with colour as black, will copy new colours to the buffers later
Shape triangle = {
    {{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
     {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
     {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
    {0, 1, 2}};


Shape square = {
    {{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
     {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
     {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
    {0, 1, 2}};

}  // namespace shapes

}  // namespace vk