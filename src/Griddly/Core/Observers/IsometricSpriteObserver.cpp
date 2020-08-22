#include "IsometricSpriteObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

IsometricSpriteObserver::IsometricSpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : SpriteObserver(grid, vulkanObserverConfig, spriteDefinitions) {
}

IsometricSpriteObserver::~IsometricSpriteObserver() {
}

void IsometricSpriteObserver::renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation renderOrientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);
  auto tileSize = vulkanObserverConfig_.tileSize;

  for (auto objectIt : objects) {
    auto object = objectIt.second;

    auto objectName = object->getObjectName();
    auto tileName = object->getObjectRenderTileName();
    auto spriteDefinition = spriteDefinitions_.at(tileName);
    auto tilingMode = spriteDefinition.tilingMode;
    auto isWallTiles = tilingMode != TilingMode::NONE;

    float objectRotationRad;
    if (object == avatarObject_ && observerConfig_.rotateWithAvatar || isWallTiles) {
      objectRotationRad = 0.0;
    } else {
      objectRotationRad = object->getObjectOrientation().getAngleRadians() - renderOrientation.getAngleRadians();
    }

    auto spriteName = getSpriteName(objectName, tileName, objectLocation, renderOrientation.getDirection());

    float outlineScale = spriteDefinition.outlineScale;

    glm::vec4 color = {1.0, 1.0, 1.0, 1.0};
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(spriteName);

    // Just a hack to keep depth between 0 and 1
    auto zCoord = (float)object->getZIdx() / 10.0;

    auto objectPlayerId = object->getPlayerId();

    if (observerConfig_.playerCount > 1 && objectPlayerId > 0) {
      auto playerId = observerConfig_.playerId;

      glm::vec4 outlineColor;

      if (playerId == objectPlayerId) {
        outlineColor = glm::vec4(0.0, 1.0, 0.0, 0.7);
      } else {
        outlineColor = globalObserverPlayerColors_[objectPlayerId - 1];
      }

      glm::vec3 position = glm::vec3(tileOffset + outputLocation + tileSize, zCoord - 1.0);
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(tileSize, 1.0));
      auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
      device_->drawSpriteOutline(ctx, spriteArrayLayer, orientedModel, outlineScale, outlineColor);
    }

    glm::vec3 position = glm::vec3(tileOffset + outputLocation + tileSize, zCoord - 1.0);
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(tileSize, 1.0));
    auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
    device_->drawSprite(ctx, spriteArrayLayer, orientedModel, color);
  }
}

void IsometricSpriteObserver::render(vk::VulkanRenderContext& ctx) const {
  auto backGroundTile = spriteDefinitions_.find("_iso_background_");
  if (backGroundTile != spriteDefinitions_.end()) {
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer("_iso_background_");
    device_->drawBackgroundTiling(ctx, spriteArrayLayer);
  }

  VulkanGridObserver::render(ctx);
}

}  // namespace griddly
