#include "IsometricSpriteObserver.hpp"

#include <cmath>

#include <glm/glm.hpp>

#include "../Grid.hpp"

namespace griddly {

IsometricSpriteObserver::IsometricSpriteObserver(std::shared_ptr<Grid> grid, IsometricSpriteObserverConfig& config)
    : SpriteObserver(std::move(grid), config) {
  config_ = config;
}

ObserverType IsometricSpriteObserver::getObserverType() const {
  return ObserverType::ISOMETRIC;
}

void IsometricSpriteObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  SpriteObserver::init(playerObservers);
}

void IsometricSpriteObserver::resetShape() {
  gridWidth_ = config_.overrideGridWidth > 0 ? config_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config_.overrideGridHeight > 0 ? config_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = config_.tileSize;

  pixelWidth_ = (gridWidth_ + gridHeight_) * tileSize.x / 2;
  pixelHeight_ = (gridWidth_ + gridHeight_) * (config_.isoTileHeight / 2) + tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};

  isoHeightRatio_ = static_cast<float>(config_.isoTileHeight) / static_cast<float>(tileSize.y);
}

glm::vec2 IsometricSpriteObserver::getTilePixelCenter(glm::ivec2 location) const {
  glm::vec2 tilePos(location.x, location.y);

  if (doTrackAvatar_ && avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    glm::vec2 v = tilePos - glm::vec2(avatarLocation);
    if (config_.rotateWithAvatar) {
      float angle = -avatarObject_->getObjectOrientation().getAngleRadians();
      float c = std::cos(angle);
      float s = std::sin(angle);
      v = glm::vec2(c * v.x - s * v.y, s * v.x + c * v.y);
    }
    tilePos = v + glm::vec2(gridWidth_ / 2.0f - 0.5f, gridHeight_ / 2.0f - 0.5f);
  }

  // Isometric shear: x' = (x - y) / 2, y' = (x + y) / 2 * heightRatio
  glm::vec2 iso(tilePos.x * 0.5f - tilePos.y * 0.5f, (tilePos.x + tilePos.y) * 0.5f * isoHeightRatio_);

  auto tileSize = config_.tileSize;
  return glm::vec2((iso.x + (gridHeight_ - 1) / 2.0f + 0.5f) * tileSize.x + config_.gridXOffset,
                   (iso.y + 0.5f) * tileSize.y + config_.gridYOffset);
}

void IsometricSpriteObserver::updateObjectRenderItems(PartialObservableGrid& observableGrid) {
  auto tileSize = config_.tileSize;
  const auto& objectIds = grid_->getObjectIds();

  auto* backgroundTexture = getSpriteTexture("_iso_background_");

  // Have to go through each location
  for (int x = observableGrid.left; x <= observableGrid.right; x++) {
    for (int y = observableGrid.bottom; y <= observableGrid.top; y++) {

      glm::ivec2 location{x, y};
      const auto& objectAtLocation = grid_->getObjectsAt(location);

      auto center = getTilePixelCenter(location);

      if (objectAtLocation.size() == 0 && backgroundTexture != nullptr) {
        SDLRenderItem background;
        background.texture = backgroundTexture;
        background.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
        background.dst = {center.x - tileSize.x / 2.0f, center.y - tileSize.y / 2.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
        background.z = -1;
        background.sortY = center.y;
        renderItems_.push_back(background);
      }

      bool backgroundDrawn = objectAtLocation.size() == 0;

      for (auto objectIt = objectAtLocation.begin(); objectIt != objectAtLocation.end(); ++objectIt) {
        auto object = objectIt->second;

        const auto& objectName = object->getObjectName();
        const auto& tileName = object->getObjectRenderTileName();
        auto objectPlayerId = object->getPlayerId();
        auto zIdx = object->getZIdx();

        if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
          throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
        }

        const auto& spriteDefinition = spriteDefinitions_.at(tileName);
        auto tileOffset = glm::vec2(spriteDefinition.offset.x / tileSize.x, spriteDefinition.offset.y / tileSize.y);
        auto tilingMode = spriteDefinition.tilingMode;
        auto isIsoFloor = tilingMode == TilingMode::ISO_FLOOR;

        if (isIsoFloor && zIdx == 0) {
          zIdx = -1;
        }

        if (objectIt == objectAtLocation.begin() && !isIsoFloor && backgroundTexture != nullptr && !backgroundDrawn) {
          SDLRenderItem background;
          background.texture = backgroundTexture;
          background.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
          background.dst = {center.x - tileSize.x / 2.0f, center.y - tileSize.y / 2.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
          background.z = -1;
          background.sortY = center.y;
          renderItems_.push_back(background);
          backgroundDrawn = true;
        }

        auto spriteName = getSpriteName(objectName, tileName, location, globalOrientation_.getDirection());
        auto* texture = getSpriteTexture(spriteName);
        if (texture == nullptr) {
          continue;
        }

        float scale = spriteDefinition.scale;

        SDLRenderItem item;
        item.texture = texture;
        item.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
        // tileOffset is in tile units
        auto offsetPixels = tileOffset * glm::vec2(tileSize);
        item.dst = {center.x - tileSize.x * scale / 2.0f + offsetPixels.x, center.y - tileSize.y * scale / 2.0f + offsetPixels.y,
                    tileSize.x * scale, tileSize.y * scale};
        item.z = static_cast<float>(zIdx);
        item.sortY = center.y;

        applyAvatarMode(item, location, objectPlayerId);

        renderItems_.push_back(item);
      }
    }
  }
}

}  // namespace griddly
