#pragma once
#include <map>
#include <memory>

#include "SDLGridObserver.hpp"

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
  glm::vec2 offset = {0, 0};
  float scale = 1.0;
};

struct SpriteObserverConfig : public SDLGridObserverConfig {
  std::map<std::string, SpriteDefinition> spriteDefinitions;
};

class SpriteObserver : public SDLGridObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, SpriteObserverConfig& config);
  ~SpriteObserver() = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

  ObserverType getObserverType() const override;

  const SpriteObserverConfig& getSpriteConfig() const { return config_; }

 protected:
  std::string getSpriteName(const std::string& objectName, const std::string& tileName, const glm::ivec2& location, Direction orientation) const;
  void updateObjectRenderItems(PartialObservableGrid& partiallyObservableGrid) override;
  void appendObjectItems(const std::shared_ptr<ObjectRules>& object, std::vector<SDLRenderItem>& out) override;

  void lazyInit() override;

  // Apply the global observer avatar mode (highlight/darken/etc.) to an item's tint
  void applyAvatarMode(SDLRenderItem& item, const glm::ivec2& location, uint32_t objectPlayerId);

  std::map<std::string, SpriteDefinition> spriteDefinitions_;
  SpriteObserverConfig config_;

  // generated texture used by HighlightStyle::GLOW / SHADOW (higher resolution than tiles so it scales smoothly)
  static constexpr const char* kHighlightGlowSprite = "__highlight_glow";
  static constexpr int32_t kHighlightGlowSize = 64;
  // SHADOW height relative to its width
  static constexpr float kHighlightShadowAspect = 0.35f;
};

}  // namespace griddly
