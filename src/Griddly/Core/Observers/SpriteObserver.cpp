#include "SpriteObserver.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <utility>

#include "../Grid.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, SpriteObserverConfig& config)
    : SDLGridObserver(std::move(grid), config) {
  config_ = config;
  spriteDefinitions_ = config_.spriteDefinitions;
}

ObserverType SpriteObserver::getObserverType() const {
  return ObserverType::SPRITE_2D;
}

void SpriteObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  SDLGridObserver::init(playerObservers);
}

void SpriteObserver::lazyInit() {
  for (auto spriteDefinitionIt : spriteDefinitions_) {
    auto spriteName = spriteDefinitionIt.first;
    auto spriteDefinition = spriteDefinitionIt.second;
    auto spriteImages = spriteDefinition.images;

    spdlog::debug("Loading sprite definition {0}", spriteName);

    if (spriteDefinition.tilingMode == TilingMode::WALL_2 || spriteDefinition.tilingMode == TilingMode::WALL_16) {
      if (spriteDefinition.tilingMode == TilingMode::WALL_2 && spriteImages.size() != 2 || spriteDefinition.tilingMode == TilingMode::WALL_16 && spriteImages.size() != 16) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectively. {0} images were supplied", spriteImages.size()));
      }

      for (int s = 0; s < spriteImages.size(); s++) {
        auto spriteNameAndIdx = spriteName + std::to_string(s);
        spdlog::debug("Loading sprite {0} image id {1}. Image: {2}", spriteName, spriteNameAndIdx, spriteDefinition.images[s]);
        loadSpriteTexture(spriteNameAndIdx, spriteDefinition.images[s], config_.tileSize);
      }
    } else {
      spdlog::debug("Loading sprite {0}. Image: {1}", spriteName, spriteDefinition.images[0]);
      loadSpriteTexture(spriteName, spriteDefinition.images[0], config_.tileSize);
    }
  }
}

std::string SpriteObserver::getSpriteName(const std::string& objectName, const std::string& tileName, const glm::ivec2& location, Direction orientation) const {
  if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
    throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
  }

  auto& tilingMode = spriteDefinitions_.at(tileName).tilingMode;

  if (tilingMode == TilingMode::WALL_2) {
    auto objectDown = grid_->getObject({location.x, location.y + 1});
    int idx = 0;
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 1;
    }

    return tileName + std::to_string(idx);

  } else if (tilingMode == TilingMode::WALL_16) {
    std::shared_ptr<ObjectRules> objectLeft, objectRight, objectUp, objectDown;
    switch (orientation) {
      case Direction::NONE:
      case Direction::UP:
        objectLeft = grid_->getObject({location.x - 1, location.y});
        objectRight = grid_->getObject({location.x + 1, location.y});
        objectUp = grid_->getObject({location.x, location.y - 1});
        objectDown = grid_->getObject({location.x, location.y + 1});
        break;
      case Direction::DOWN:
        objectLeft = grid_->getObject({location.x + 1, location.y});
        objectRight = grid_->getObject({location.x - 1, location.y});
        objectUp = grid_->getObject({location.x, location.y + 1});
        objectDown = grid_->getObject({location.x, location.y - 1});
        break;
      case Direction::LEFT:
        objectLeft = grid_->getObject({location.x, location.y + 1});
        objectRight = grid_->getObject({location.x, location.y - 1});
        objectUp = grid_->getObject({location.x - 1, location.y});
        objectDown = grid_->getObject({location.x + 1, location.y});
        break;
      case Direction::RIGHT:
        objectLeft = grid_->getObject({location.x, location.y - 1});
        objectRight = grid_->getObject({location.x, location.y + 1});
        objectUp = grid_->getObject({location.x + 1, location.y});
        objectDown = grid_->getObject({location.x - 1, location.y});
        break;
      default:
        objectLeft = grid_->getObject({location.x - 1, location.y});
        objectRight = grid_->getObject({location.x + 1, location.y});
        objectUp = grid_->getObject({location.x, location.y - 1});
        objectDown = grid_->getObject({location.x, location.y + 1});
        break;
    }

    int idx = 0;
    if (objectLeft != nullptr && objectLeft->getObjectName() == objectName) {
      idx += 1;
    }
    if (objectRight != nullptr && objectRight->getObjectName() == objectName) {
      idx += 2;
    }
    if (objectUp != nullptr && objectUp->getObjectName() == objectName) {
      idx += 4;
    }
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 8;
    }

    return tileName + std::to_string(idx);
  }

  return tileName;
}

void SpriteObserver::applyAvatarMode(SDLRenderItem& item, const glm::ivec2& location, uint32_t objectPlayerId) {
  if (config_.globalObserverAvatarMode == GlobalObserverAvatarMode::NONE || avatarObject_ == nullptr) {
    return;
  }

  // Is this location visible to any player?
  bool inPlayerView = false;
  for (auto& weakObserver : playerObservers_) {
    auto playerObserver = weakObserver.lock();
    if (playerObserver == nullptr) continue;
    const auto& og = playerObserver->getObservableGrid();
    if (location.x >= og.left && location.x <= og.right && location.y >= og.bottom && location.y <= og.top) {
      inPlayerView = true;
      break;
    }
  }

  switch (config_.globalObserverAvatarMode) {
    case GlobalObserverAvatarMode::HIGHLIGHT_VISIBLE:
      if (inPlayerView) {
        auto hc = config_.globalObserverAvatarHighlightColor;
        item.r = static_cast<uint8_t>(std::min(1.0f, item.r / 255.0f + hc.r) * 255.0f);
        item.g = static_cast<uint8_t>(std::min(1.0f, item.g / 255.0f + hc.g) * 255.0f);
        item.b = static_cast<uint8_t>(std::min(1.0f, item.b / 255.0f + hc.b) * 255.0f);
      }
      break;
    case GlobalObserverAvatarMode::DARKEN_INVISIBLE:
      if (!inPlayerView) {
        item.r = static_cast<uint8_t>(item.r * 0.4f);
        item.g = static_cast<uint8_t>(item.g * 0.4f);
        item.b = static_cast<uint8_t>(item.b * 0.4f);
      }
      break;
    case GlobalObserverAvatarMode::GRAYSCALE_INVISIBLE:
      if (!inPlayerView) {
        float grey = (item.r * 0.299f + item.g * 0.587f + item.b * 0.114f) / 255.0f;
        item.r = item.g = item.b = static_cast<uint8_t>(grey * 255.0f);
      }
      break;
    case GlobalObserverAvatarMode::REMOVE_INVISIBLE:
      if (!inPlayerView) {
        item.a = 0;
      }
      break;
    default:
      break;
  }
}

void SpriteObserver::updateObjectRenderItems(PartialObservableGrid& observableGrid) {
  auto tileSize = config_.tileSize;

  auto* backgroundTexture = getSpriteTexture("_background_");
  if (backgroundTexture != nullptr) {
    SDLRenderItem background;
    background.texture = backgroundTexture;
    background.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
    background.dst = {0, 0, static_cast<float>(pixelWidth_), static_cast<float>(pixelHeight_)};
    background.tile = true;
    background.z = -10;
    renderItems_.push_back(background);
  }

  const auto& objects = grid_->getObjects();

  // Padding tiles outside the grid boundary
  auto* paddingTexture = getSpriteTexture("_padding_");
  if (paddingTexture != nullptr) {
    for (int32_t xPad = observableGrid.right - gridWidth_; xPad < observableGrid.left + static_cast<int32_t>(gridWidth_); xPad++) {
      for (int32_t yPad = observableGrid.top - gridHeight_; yPad < observableGrid.bottom + static_cast<int32_t>(gridHeight_); yPad++) {
        if (xPad < 0 || yPad < 0 || xPad >= gridBoundary_.x || yPad >= gridBoundary_.y) {
          auto center = getTilePixelCenter({xPad, yPad});
          SDLRenderItem padding;
          padding.texture = paddingTexture;
          padding.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
          padding.dst = {center.x - tileSize.x / 2.0f, center.y - tileSize.y / 2.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
          padding.z = -10;
          renderItems_.push_back(padding);
        }
      }
    }
  }

  for (auto& object : objects) {
    const auto& location = object->getLocation();

    if (location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top) {
      continue;
    }

    const auto& objectName = object->getObjectName();
    const auto& tileName = object->getObjectRenderTileName();
    auto objectPlayerId = object->getPlayerId();
    auto zIdx = object->getZIdx();

    if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
      throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
    }

    const auto& spriteDefinition = spriteDefinitions_.at(tileName);
    auto tilingMode = spriteDefinition.tilingMode;
    auto isWallTiles = tilingMode != TilingMode::NONE;

    auto spriteName = getSpriteName(objectName, tileName, location, globalOrientation_.getDirection());
    auto* texture = getSpriteTexture(spriteName);
    if (texture == nullptr) {
      continue;
    }

    auto center = getTilePixelCenter(location);
    float scale = spriteDefinition.scale;

    SDLRenderItem item;
    item.texture = texture;
    item.src = {0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
    item.dst = {center.x - tileSize.x * scale / 2.0f, center.y - tileSize.y * scale / 2.0f,
                tileSize.x * scale, tileSize.y * scale};
    item.z = static_cast<float>(zIdx);
    item.sortY = center.y;

    if (!isWallTiles) {
      item.angle = getObjectAngle(object, globalOrientation_);
    }

    // Approximate the old shader's player-color outline with a slightly larger quad behind the sprite
    if (config_.highlightPlayers) {
      auto playerColorId = getEgocentricPlayerId(objectPlayerId);
      glm::vec4 playerColor = (playerColorId >= 1 && playerColorId <= playerColors_.size()) ? playerColors_[playerColorId - 1] : glm::vec4(1.0f);
      SDLRenderItem outline = item;
      outline.r = static_cast<uint8_t>(playerColor.r * 255.0f);
      outline.g = static_cast<uint8_t>(playerColor.g * 255.0f);
      outline.b = static_cast<uint8_t>(playerColor.b * 255.0f);
      outline.dst = {center.x - tileSize.x * scale * 1.1f / 2.0f, center.y - tileSize.y * scale * 1.1f / 2.0f,
                     tileSize.x * scale * 1.1f, tileSize.y * scale * 1.1f};
      outline.z = static_cast<float>(zIdx) - 0.5f;
      renderItems_.push_back(outline);
    }

    applyAvatarMode(item, location, objectPlayerId);

    renderItems_.push_back(item);
  }
}

}  // namespace griddly
