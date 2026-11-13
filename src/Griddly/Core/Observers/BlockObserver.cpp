#include "BlockObserver.hpp"

#include <utility>

#include "../Grid.hpp"

namespace griddly {

const std::map<std::string, SpriteDefinition> BlockObserver::blockSpriteDefinitions_ = {
    {"circle", {{"block_shapes/circle.png"}}},
    {"triangle", {{"block_shapes/triangle.png"}}},
    {"square", {{"block_shapes/square.png"}}},
    {"pentagon", {{"block_shapes/pentagon.png"}}},
    {"hexagon", {{"block_shapes/hexagon.png"}}},
    {"fill", {{"block_shapes/fill.png"}}},
};

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, BlockObserverConfig& config)
    : SpriteObserver(std::move(grid), config) {
  blockDefinitions_ = config.blockDefinitions;
  config_ = config;
  // The background is a solid clear color rather than a tiled texture
  clearColor_ = config_.backgroundColor;
}

ObserverType BlockObserver::getObserverType() const {
  return ObserverType::BLOCK_2D;
}

void BlockObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  SpriteObserver::init(playerObservers);
}

void BlockObserver::updateObjectRenderItems(PartialObservableGrid& observableGrid) {
  const auto& objects = grid_->getObjects();

  for (auto& object : objects) {
    auto location = object->getLocation();

    if (!(location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top)) {
      appendObjectItems(object, renderItems_);
    }
  }
}

void BlockObserver::appendObjectItems(const std::shared_ptr<ObjectRules>& object, std::vector<SDLRenderItem>& out) {
  auto tileSize = config_.tileSize;
  auto location = object->getLocation();
  const auto& tileName = object->getObjectRenderTileName();
  auto objectPlayerId = object->getPlayerId();
  auto zIdx = object->getZIdx();

  if (fogHides(object)) {
    return;
  }

  const auto& blockDefinition = blockDefinitions_.at(tileName);

  auto center = getTilePixelCenter(location);
  float scale = blockDefinition.scale;

  SDLRenderItem item;
  item.texture = getSpriteTexture(blockDefinition.shape);
  if (item.texture == nullptr) {
    return;
  }
  item.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
  item.dst = {center.x - tileSize.x * scale / 2.0f, center.y - tileSize.y * scale / 2.0f,
              tileSize.x * scale, tileSize.y * scale};
  item.angle = getObjectAngle(object, globalOrientation_);
  item.z = static_cast<float>(zIdx);
  item.sortY = center.y;

  if (blockDefinition.usePlayerColor) {
    auto playerColorId = getEgocentricPlayerId(objectPlayerId);
    glm::vec4 playerColor = (playerColorId >= 1 && playerColorId <= playerColors_.size()) ? playerColors_[playerColorId - 1] : glm::vec4(1.0f);
    item.r = static_cast<uint8_t>(playerColor.r * 255.0f);
    item.g = static_cast<uint8_t>(playerColor.g * 255.0f);
    item.b = static_cast<uint8_t>(playerColor.b * 255.0f);
  } else {
    item.r = static_cast<uint8_t>(blockDefinition.color[0] * 255.0f);
    item.g = static_cast<uint8_t>(blockDefinition.color[1] * 255.0f);
    item.b = static_cast<uint8_t>(blockDefinition.color[2] * 255.0f);
  }

  applyAvatarMode(item, location, objectPlayerId);

  out.push_back(item);
  appendHealthBar(object, center, out);
}

}  // namespace griddly
