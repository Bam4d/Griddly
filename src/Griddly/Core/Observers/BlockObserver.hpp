#pragma once
#include <glm/glm.hpp>
#include <map>
#include <memory>

#include "SpriteObserver.hpp"

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
  glm::vec3 backgroundColor{};
};

class BlockObserver : public SpriteObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, BlockObserverConfig& config);
  ~BlockObserver() override = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

  ObserverType getObserverType() const override;
  void updateObjectRenderItems(PartialObservableGrid& partiallyObservableGrid) override;

  const static std::map<std::string, SpriteDefinition> blockSpriteDefinitions_;

 private:
  std::map<std::string, BlockDefinition> blockDefinitions_;
  BlockObserverConfig config_;
};

}  // namespace griddly
