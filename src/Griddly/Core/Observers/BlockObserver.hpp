#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "SpriteObserver.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

struct BlockDefinition {
  glm::vec3 color{1.0, 1.0, 1.0};
  std::string shape = "";
  float scale = 1.0;
  float outlineScale = 1.0;
  bool usePlayerColor = false;
};

struct BlockObserverConfig : public SpriteObserverConfig {
  std::map<std::string, BlockDefinition> blockDefinitions;
};

class BlockObserver : public SpriteObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, BlockObserverConfig& config);
  ~BlockObserver() override = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

  ObserverType getObserverType() const override;
  void updateObjectSSBOData(PartialObservableGrid& partiallyObservableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) override;

  const static std::map<std::string, SpriteDefinition> blockSpriteDefinitions_;

 private:
  void updateObjectSSBOs(std::vector<vk::ObjectSSBOs>& objectSSBOCache, std::shared_ptr<Object> object, glm::mat4& globalModelMatrix, DiscreteOrientation& globalOrientation);
  std::map<std::string, BlockDefinition> blockDefinitions_;

  

  BlockObserverConfig config_;
};

}  // namespace griddly
