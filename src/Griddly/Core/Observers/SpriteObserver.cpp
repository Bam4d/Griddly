#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : VulkanGridObserver(grid, resourceConfig), spriteDefinitions_(spriteDefinitions) {
}

SpriteObserver::~SpriteObserver() {
}

ObserverType SpriteObserver::getObserverType() const {
  return ObserverType::SPRITE_2D;
}

// Load a single texture
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;

  std::string absoluteFilePath = resourceConfig_.imagePath + "/" + imageFilename;

  spdlog::debug("Loading Sprite {0}", absoluteFilePath);

  stbi_uc* pixels = stbi_load(absoluteFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image.");
  }

  int outputWidth = observerConfig_.tileSize.x;
  int outputHeight = observerConfig_.tileSize.y;

  stbi_uc* resizedPixels = (stbi_uc*)malloc(outputWidth * outputHeight * 4);

  auto res = stbir_resize_uint8_generic(pixels, width, height, 0,
                                        resizedPixels, outputWidth, outputHeight, 0, 4,
                                        3,
                                        0,
                                        STBIR_EDGE_CLAMP,
                                        STBIR_FILTER_CATMULLROM,
                                        STBIR_COLORSPACE_LINEAR,
                                        nullptr);

  free(pixels);

  if (!res) {
    throw std::runtime_error("Failed to load texture image.");
  }

  spdlog::debug("Sprite loaded: {0}, width={1}, height={2}. channels={3}", absoluteFilePath, width, height, channels);

  std::unique_ptr<uint8_t[]> spriteData(resizedPixels);

  return {std::move(spriteData), (uint32_t)outputWidth, (uint32_t)outputHeight, (uint32_t)4};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(ObserverConfig observerConfig) {
  VulkanObserver::init(observerConfig);

  device_->initRenderMode(vk::RenderMode::SPRITES);

  std::unordered_map<std::string, vk::SpriteData> spriteData;
  for (auto spriteDefinitionIt : spriteDefinitions_) {
    auto spriteDefinition = spriteDefinitionIt.second;
    auto spriteName = spriteDefinitionIt.first;
    auto spriteImages = spriteDefinition.images;

    if (spriteDefinition.tilingMode == TilingMode::WALL_2 || spriteDefinition.tilingMode == TilingMode::WALL_16) {
      if (spriteDefinition.tilingMode == TilingMode::WALL_2 && spriteImages.size() != 2 || spriteDefinition.tilingMode == TilingMode::WALL_16 && spriteImages.size() != 16) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectively. {0} images were supplied", spriteImages.size()));
      }

      for (int s = 0; s < spriteImages.size(); s++) {
        auto spriteNameAndIdx = spriteName + std::to_string(s);
        spriteData.insert({spriteNameAndIdx, loadImage(spriteDefinition.images[s])});
      }
    } else {
      spriteData.insert({spriteName, loadImage(spriteDefinition.images[0])});
    }
  }

  device_->preloadSprites(spriteData);

  resetRenderSurface();
}

std::string SpriteObserver::getSpriteName(std::string objectName, std::string tileName, glm::ivec2 location, Direction orientation) const {
  auto tilingMode = spriteDefinitions_.at(tileName).tilingMode;

  if (tilingMode == TilingMode::WALL_2) {
    auto objectDown = grid_->getObject({location.x, location.y + 1});
    int idx = 0;
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 1;
    }

    return tileName + std::to_string(idx);

  } else if (tilingMode == TilingMode::WALL_16) {
    std::shared_ptr<Object> objectLeft, objectRight, objectUp, objectDown;
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

void SpriteObserver::renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation renderOrientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);
  auto tileSize = observerConfig_.tileSize;

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

      glm::vec3 position = glm::vec3(tileOffset + outputLocation * tileSize, zCoord - 1.0);
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(tileSize, 1.0));
      auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
      device_->drawSpriteWithOutline(ctx, spriteArrayLayer, orientedModel, color, outlineColor);
    } else {
      glm::vec3 position = glm::vec3(tileOffset + outputLocation * tileSize, zCoord - 1.0);
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(tileSize, 1.0));
      auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
      device_->drawSprite(ctx, spriteArrayLayer, orientedModel, color);
    }
  }
}

void SpriteObserver::render(vk::VulkanRenderContext& ctx) const {
  auto backGroundTile = spriteDefinitions_.find("_background_");
  if (backGroundTile != spriteDefinitions_.end()) {
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer("_background_");
    device_->drawBackgroundTiling(ctx, spriteArrayLayer);
  }

  VulkanGridObserver::render(ctx);
}

}  // namespace griddly
