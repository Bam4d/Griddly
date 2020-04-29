#pragma once
#include <memory>

#include "Vulkan/VulkanObserver.hpp"
#include <glm/glm.hpp>
#include "Vulkan/VulkanDevice.hpp"


namespace griddy {

struct BlockDefinition {
  float color[3];
  std::string shape;
  float scale;
};

struct BlockConfig {
  glm::vec3 color;
  vk::ShapeBuffer shapeBuffer;
  float scale;
};

class BlockObserver : public VulkanObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, uint32_t tileSize, std::unordered_map<std::string, BlockDefinition> blockDefinitions);
  ~BlockObserver();

  void init(uint gridWidth, uint gridHeight) override;

  std::unique_ptr<uint8_t[]> update(int playerId) const override;
  std::unique_ptr<uint8_t[]> reset() const override;

 private:
  void render(vk::VulkanRenderContext& ctx) const;

  std::unordered_map<std::string, BlockConfig> blockConfigs_;
  const std::unordered_map<std::string, BlockDefinition> blockDefinitions_;
};

}  // namespace griddy