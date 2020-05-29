#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : VulkanGridObserver(grid, vulkanObserverConfig), spriteDefinitions_(spriteDefinitions) {
}

SpriteObserver::~SpriteObserver() {
}

// Load a single texture
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;

  std::string absoluteFilePath = vulkanObserverConfig_.resourcePath + "/" + imageFilename;

  spdlog::debug("Loading Sprite {0}", absoluteFilePath);

  stbi_uc* pixels = stbi_load(absoluteFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image.");
  }

  spdlog::debug("Sprite loaded: {0}, width={1}, height{2}. channels={3}", absoluteFilePath, width, height, channels);

  auto spriteSize = width * height * channels;

  std::unique_ptr<uint8_t[]> spriteData(pixels);

  return {std::move(spriteData), (uint32_t)width, (uint32_t)height, (uint32_t)4};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(ObserverConfig observerConfig) {
  VulkanObserver::init(observerConfig);

  std::unordered_map<std::string, vk::SpriteData> spriteData;
  for (auto spriteDefinitionIt : spriteDefinitions_) {
    auto spriteDefinition = spriteDefinitionIt.second;
    auto spriteName = spriteDefinitionIt.first;
    auto spriteImages = spriteDefinition.images;

    if (spriteDefinition.tilingMode != TilingMode::NONE) {
      if (spriteDefinition.tilingMode == TilingMode::WALL_2 && spriteImages.size() != 2 || spriteDefinition.tilingMode == TilingMode::WALL_16 && spriteImages.size() != 16) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectivtely. {0} images were supplied", spriteImages.size()));
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

  device_->initRenderMode(vk::RenderMode::SPRITES);
}

std::string SpriteObserver::getSpriteName(std::string objectName, GridLocation location, Direction orientation) const {
  auto tilingMode = spriteDefinitions_.at(objectName).tilingMode;

  if (tilingMode == TilingMode::NONE) {
    return objectName;
  } else if (tilingMode == TilingMode::WALL_2) {
    auto objectDown = grid_->getObject({location.x, location.y + 1});
    int idx = 0;
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 1;
    }

    return objectName + std::to_string(idx);

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
        objectLeft = grid_->getObject({location.x - 1, location.y});
        objectRight = grid_->getObject({location.x + 1, location.y});
        objectUp = grid_->getObject({location.x, location.y + 1});
        objectDown = grid_->getObject({location.x, location.y - 1});
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

    return objectName + std::to_string(idx);
  }
}

void SpriteObserver::renderLocation(vk::VulkanRenderContext& ctx, GridLocation objectLocation, GridLocation outputLocation, float scale, float tileOffset, Direction orientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);

  // Have to use a reverse iterator
  for (auto objectIt = objects.begin(); objectIt != objects.end(); objectIt++) {
    auto object = objectIt->second;

    auto objectName = object->getObjectName();
    auto objectRotationRad = getObjectRotation(object);

    auto spriteName = getSpriteName(objectName, objectLocation, orientation);

    glm::vec3 color = {1.0, 1.0, 1.0};
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(spriteName);

    // Just a hack to keep depth between 0 and 1
    auto zCoord = (float)object->getZIdx() / 10.0;

    glm::vec3 position = {tileOffset + outputLocation.x * scale, tileOffset + outputLocation.y * scale, zCoord - 1.0};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
    auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
    device_->drawSprite(ctx, spriteArrayLayer, orientedModel, color);
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
