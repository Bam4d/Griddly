#pragma once
#include <memory>

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

class SpriteObserver : public VulkanGridObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDesciptions, ShaderVariableConfig shaderVariableConfig);
  ~SpriteObserver();

  virtual ObserverType getObserverType() const override;
  void updateCommandBuffer(std::vector<vk::ObjectDataSSBO> objectData) override;

 protected:
  std::string getSpriteName(std::string objectName, std::string tileName, glm::ivec2 location, Direction orientation) const;
  std::unordered_map<std::string, SpriteDefinition> spriteDefinitions_;

  std::vector<vk::ObjectSSBOs> updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  vk::SpriteData loadImage(std::string imageFilename);

  void lazyInit() override;

};

}  // namespace griddly