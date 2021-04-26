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
  WALL_16,
  ISO_FLOOR,
};

struct SpriteDefinition {
  std::vector<std::string> images;
  TilingMode tilingMode = TilingMode::NONE;
  float outlineScale = 2.0f;
  glm::vec2 offset = {0, 0};
};

class SpriteObserver : public VulkanGridObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~SpriteObserver();

  virtual ObserverType getObserverType() const override;

 protected:
  void renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation orientation) const override;
  void render(vk::VulkanRenderContext& ctx) const override;
  std::string getSpriteName(std::string objectName, std::string tileName, glm::ivec2 location, Direction orientation) const;
  std::unordered_map<std::string, SpriteDefinition> spriteDefinitions_;

 private:
  vk::SpriteData loadImage(std::string imageFilename);

  void lazyInit() override;
};

}  // namespace griddly