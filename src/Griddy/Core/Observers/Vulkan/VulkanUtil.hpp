#include <vulkan/vulkan.h>
#include <cassert>

namespace vk {
inline bool vk_check(VkResult res) {
  assert(("Vulkan command did not execute correctly", res == VK_SUCCESS));
}
}  // namespace vk