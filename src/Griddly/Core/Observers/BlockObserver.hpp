#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Vulkan/VulkanDevice.hpp"
#include "SpriteObserver.hpp"

namespace griddly {

struct BlockDefinition {
  glm::vec3 color{1.0,1.0,1.0};
  std::string shape = "";
  float scale = 1.0;
  float outlineScale = 1.0;
  bool usePlayerColor = false;
};

struct BlockObserverConfig : public SpriteObserverConfig {
  std::unordered_map<std::string, BlockDefinition> blockDefinitions;
};


class BlockObserver : public SpriteObserver, public ObserverConfigInterface<BlockObserverConfig> {
 public:
  BlockObserver(std::shared_ptr<Grid> grid);
  ~BlockObserver() override = default;

  void init(BlockObserverConfig& config) override;

  ObserverType getObserverType() const override;
  void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

 private:
  void updateObjectSSBOs(std::vector<vk::ObjectSSBOs>& objectSSBOCache, std::shared_ptr<Object> object, glm::mat4& globalModelMatrix, DiscreteOrientation& globalOrientation);
  std::unordered_map<std::string, BlockDefinition> blockDefinitions_;

  const static std::unordered_map<std::string, SpriteDefinition> blockSpriteDefinitions_;


  BlockObserverConfig config_;
};

}  // namespace griddly
