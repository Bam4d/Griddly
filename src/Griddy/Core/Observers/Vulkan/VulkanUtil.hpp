#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <cassert>

namespace vk {
inline bool vk_check(VkResult res) {
  assert(("Vulkan command did not execute correctly", res == VK_SUCCESS));
}

VkShaderModule loadShader(const char* fileName, VkDevice device) {
  std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    size_t size = is.tellg();
    is.seekg(0, std::ios::beg);
    char* shaderCode = new char[size];
    is.read(shaderCode, size);
    is.close();

    assert(size > 0);

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (uint32_t*)shaderCode;

    vk_check(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

    delete[] shaderCode;

    return shaderModule;
  } else {
    spdlog::error("Error: Could not open shader file {0}", fileName);
    return VK_NULL_HANDLE;
  }
}

}  // namespace vk