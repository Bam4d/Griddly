#include <vulkan/vulkan.h>
namespace vk {

struct VertexIndexBuffer {
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
};

struct ShapeBuffers {
  VertexIndexBuffer square;
  VertexIndexBuffer triangle;
};
}  // namespace vk