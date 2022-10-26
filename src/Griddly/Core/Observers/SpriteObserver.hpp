#pragma once
#include <memory>
#include <map>

#include "VulkanGridObserver.hpp"

namespace vk {
struct SpriteData;
struct ShapeBuffer;
}  // namespace vk

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

struct SpriteObserverConfig : public VulkanGridObserverConfig {
  std::map<std::string, SpriteDefinition> spriteDefinitions;
};

class SpriteObserver : public VulkanGridObserver, public ObserverConfigInterface<SpriteObserverConfig> {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid);
  ~SpriteObserver() = default;

  void init(SpriteObserverConfig& config) override;

  ObserverType getObserverType() const override;
  void updateCommandBuffer() override;

 protected:
  std::string getSpriteName(const std::string& objectName, const std::string& tileName, const glm::ivec2& location, Direction orientation) const;
  std::map<std::string, SpriteDefinition> spriteDefinitions_;

  void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  vk::SpriteData loadImage(std::string imageFilename);

  void lazyInit() override;

  SpriteObserverConfig config_;
};

}  // namespace griddly
