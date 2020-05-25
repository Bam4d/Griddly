#pragma once
#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace vk {
struct SpriteData;
}

namespace griddly {

enum class TilingMode {
  NONE,
  WALL_2,
  WALL_16
};

struct SpriteDefinition {
  std::vector<std::string> images;
  TilingMode tilingMode = TilingMode::NONE;
};

class SpriteObserver : public VulkanObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, uint32_t tileSize, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions, std::string resourcePath);
  ~SpriteObserver();

  std::shared_ptr<uint8_t> update(int playerId) const override;
  std::shared_ptr<uint8_t> reset() const override;

  void init(uint32_t gridWidth, uint32_t gridHeight) override;

 private:
  vk::SpriteData loadImage(std::string imageFilename);

  std::string getSpriteName(std::string objectName, GridLocation location) const;
  void render(vk::VulkanRenderContext& ctx) const;
  std::unordered_map<std::string, SpriteDefinition> spriteDefinitions_;
};

}  // namespace griddly