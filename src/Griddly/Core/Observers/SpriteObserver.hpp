#pragma once
#include <memory>

#include "VulkanGridObserver.hpp"

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
  float outlineScale = 2.0f;
};

class SpriteObserver : public VulkanGridObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~SpriteObserver();

  void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, float tileOffset, DiscreteOrientation orientation) const override;

  void init(ObserverConfig observerConfig) override;

protected:
  void render(vk::VulkanRenderContext& ctx) const override;
private:
  vk::SpriteData loadImage(std::string imageFilename);
  std::string getSpriteName(std::string objectName, glm::ivec2 location, Direction orientation) const;
  std::unordered_map<std::string, SpriteDefinition> spriteDefinitions_;
};

}  // namespace griddly