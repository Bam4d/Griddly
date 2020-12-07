#include "IsometricSpriteObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

IsometricSpriteObserver::IsometricSpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : SpriteObserver(grid, resourceConfig, spriteDefinitions) {
}

IsometricSpriteObserver::~IsometricSpriteObserver() {
}

ObserverType IsometricSpriteObserver::getObserverType() const {
  return ObserverType::ISOMETRIC;
}

void IsometricSpriteObserver::resetShape() {
  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto tileSize = observerConfig_.tileSize;
  auto isoTileYOffset = observerConfig_.isoTileYOffset;

  auto offsetTileHeight = tileSize.y - isoTileYOffset;

  pixelWidth_ = (gridWidth_ + gridHeight_) * tileSize.x / 2;
  pixelHeight_ = (gridWidth_ + gridHeight_) * isoTileYOffset / 2 + offsetTileHeight;

  auto offsetDim = std::min(gridWidth_, gridHeight_);
  isoOriginOffset_ = {(offsetDim)*tileSize.x / 2, observerConfig_.isoTileYOffset};

  observationShape_ = {3, pixelWidth_, pixelHeight_};
  observationStrides_ = {1, 3, 3 * pixelWidth_};
}

std::vector<VkRect2D> IsometricSpriteObserver::calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const {
  auto tileSize = observerConfig_.tileSize;
  std::vector<VkRect2D> dirtyRectangles;

  const glm::ivec2 noOffset = {0, 0};

  for (auto location : updatedLocations) {
    // If the observation window is smaller than the actual grid for some reason, dont try to render the off-image things
    if (gridHeight_ <= location.y || gridWidth_ <= location.x) {
      continue;
    }

    glm::vec2 isometricLocation = isometricOutputLocation(location, noOffset);

    VkOffset2D offset = {
        std::max(0, (int32_t)isometricLocation.x - (tileSize.x / 2) - 2),
        std::max(0, (int32_t)isometricLocation.y - (int32_t)observerConfig_.isoTileYOffset - 2)};

    // Because we make the dirty rectangles slightly larger than the sprites, must check boundaries do not go beyond
    // the render image surface
    auto extentWidth = (uint32_t)tileSize.x + 4;
    auto boundaryX = (int32_t)extentWidth + offset.x - (int32_t)pixelWidth_;
    if (boundaryX > 0) {
      extentWidth -= boundaryX;
    }

    auto extentHeight = (uint32_t)tileSize.y + 4;
    auto boundaryY = (int32_t)extentHeight + offset.y - (int32_t)pixelHeight_;
    if (boundaryY > 0) {
      extentHeight -= boundaryY;
    }

    VkExtent2D extent;
    extent.width = extentWidth;
    extent.height = extentHeight;

    dirtyRectangles.push_back({offset, extent});
  }

  return dirtyRectangles;
}

void IsometricSpriteObserver::renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation renderOrientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);
  auto tileSize = observerConfig_.tileSize;

  uint32_t backgroundSpriteArrayLayer = device_->getSpriteArrayLayer("_iso_background_");
  const glm::vec4 color = {1.0, 1.0, 1.0, 1.0};

  for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
    auto object = objectIt->second;

    auto objectName = object->getObjectName();
    auto tileName = object->getObjectRenderTileName();
    auto spriteDefinition = spriteDefinitions_.at(tileName);
    auto tilingMode = spriteDefinition.tilingMode;
    auto isIsoFloor = tilingMode == TilingMode::ISO_FLOOR;

    float outlineScale = spriteDefinition.outlineScale;

    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(tileName);

    // Just a hack to keep depth between 0 and 1
    auto zCoord = (float)object->getZIdx() / 10.0;

    auto objectPlayerId = object->getPlayerId();

    auto isometricCoords = isometricOutputLocation(outputLocation, spriteDefinition.offset);
    glm::vec3 position = glm::vec3(isometricCoords, zCoord - 1.0);
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3((glm::vec2)tileSize, 1.0));

    // if we dont have a floor tile, but its the first tile in the list, add a default floor tile
    if (objectIt == objects.begin() && !isIsoFloor) {
      device_->drawSprite(ctx, backgroundSpriteArrayLayer, model, color);
    }

    if (observerConfig_.playerCount > 1 && objectPlayerId > 0) {
      auto playerId = observerConfig_.playerId;

      glm::vec4 outlineColor;

      if (playerId == objectPlayerId) {
        outlineColor = glm::vec4(0.0, 1.0, 0.0, 0.7);
      } else {
        outlineColor = globalObserverPlayerColors_[objectPlayerId - 1];
      }

      device_->drawSprite(ctx, spriteArrayLayer, model, color, outlineColor);
    } else {
      device_->drawSprite(ctx, spriteArrayLayer, model, color);
    }

    

    
  }

  // If there's actually nothing at this location just draw background tile
  if (objects.size() == 0) {
    auto spriteDefinition = spriteDefinitions_.at("_iso_background_");
    auto isometricCoords = isometricOutputLocation(outputLocation, spriteDefinition.offset);
    glm::vec3 position = glm::vec3(isometricCoords, -1.0);
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3((glm::vec2)tileSize, 1.0));
    device_->drawSprite(ctx, backgroundSpriteArrayLayer, model, color);
  }
}

glm::vec2 IsometricSpriteObserver::isometricOutputLocation(glm::vec2 outputLocation, glm::vec2 offset) const {
  auto tileSize = observerConfig_.tileSize;

  auto tilePosition = (glm::vec2)tileSize / 2.0f;
  tilePosition.y -= observerConfig_.isoTileYOffset;

  const glm::mat2 isoMat = {
      {1.0, -1.0},
      {1.0, 1.0},
  };

  return offset + isoOriginOffset_ + outputLocation * isoMat * tilePosition;
}

}  // namespace griddly
