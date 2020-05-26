#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : VulkanObserver(grid, vulkanObserverConfig), spriteDefinitions_(spriteDefinitions) {
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

std::shared_ptr<uint8_t> SpriteObserver::reset() const {
  auto tileSize = vulkanObserverConfig_.tileSize;
  auto ctx = device_->beginRender();

  render(ctx);

  auto width = observerConfig_.width * tileSize;
  auto height = observerConfig_.height * tileSize;

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles = {
      {{0, 0},
       {width, height}}};

  return device_->endRender(ctx, dirtyRectangles);
}

std::shared_ptr<uint8_t> SpriteObserver::update(int playerId) const {
  auto ctx = device_->beginRender();
  auto tileSize = vulkanObserverConfig_.tileSize;

  render(ctx);

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles;

  auto updatedLocations = grid_->getUpdatedLocations();
  auto avatarLocation = avatarObject_->getLocation();
  auto observationRect = getPartialObservationRect();

  for (auto l : updatedLocations) {
    VkOffset2D offset;
    VkExtent2D extent;
    if (observerConfig_.trackAvatar) {

      if (l.x > observationRect.left && l.x < observationRect.right && l.y > observationRect.bottom && l.y < observationRect.top) {
        auto x = l.x - avatarLocation.x;
        auto y = l.y - avatarLocation.y;
        offset = {(int32_t)(x * tileSize), (int32_t)(y * tileSize)};
        extent = {tileSize, tileSize};
        dirtyRectangles.push_back({offset, extent});
      }

    } else {
      offset = {(int32_t)(l.x * tileSize), (int32_t)(l.y * tileSize)};
      extent = {tileSize, tileSize};
      dirtyRectangles.push_back({offset, extent});
    }
  }

  return device_->endRender(ctx, dirtyRectangles);
}

std::string SpriteObserver::getSpriteName(std::string objectName, GridLocation location) const {
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
    auto objectLeft = grid_->getObject({location.x - 1, location.y});
    auto objectRight = grid_->getObject({location.x + 1, location.y});
    auto objectUp = grid_->getObject({location.x, location.y - 1});
    auto objectDown = grid_->getObject({location.x, location.y + 1});
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

void SpriteObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = observerConfig_.tileSize;
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto backGroundTile = spriteDefinitions_.find("_background_");
  if (backGroundTile != spriteDefinitions_.end()) {
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer("_background_");
    device_->drawBackgroundTiling(ctx, spriteArrayLayer);
  }

  auto offset = (float)tileSize / 2.0f;

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto location : updatedLocations) {
    auto objects = grid_->getObjectsAt(location);

    // Have to use a reverse iterator
    for (auto objectIt = objects.begin(); objectIt != objects.end(); objectIt++) {
      auto object = objectIt->second;

      float scale = (float)tileSize;
      auto objectName = object->getObjectName();

      auto spriteName = getSpriteName(objectName, location);

      glm::vec3 color = {1.0, 1.0, 1.0};
      uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(spriteName);

      // Just a hack to keep depth between 0 and 1
      auto zCoord = (float)object->getZIdx() / 10.0;

      glm::vec3 position = {offset + location.x * tileSize, offset + location.y * tileSize, zCoord - 1.0};
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
      device_->drawSprite(ctx, spriteArrayLayer, model, color);
    }
  }
}

}  // namespace griddly
