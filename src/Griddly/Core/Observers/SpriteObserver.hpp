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
};

class SpriteObserver : public VulkanGridObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions);
  ~SpriteObserver();

  void renderLocation(vk::VulkanRenderContext& ctx, GridLocation objectLocation, GridLocation outputLocation, float scale, float tileOffset, Direction orientation) const override;

  void init(ObserverConfig observerConfig) override;

protected:
  void render(vk::VulkanRenderContext& ctx) const override;
private:
  vk::SpriteData loadImage(std::string imageFilename);
  std::string getSpriteName(std::string objectName, GridLocation location, Direction orientation) const;
  std::unordered_map<std::string, SpriteDefinition> spriteDefinitions_;
};

}  // namespace griddly