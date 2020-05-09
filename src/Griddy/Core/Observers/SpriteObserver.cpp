#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddy {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, uint32_t spriteSize, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions) : VulkanObserver(grid, spriteSize), spriteDefinitions_(spriteDefinitions) {
}

SpriteObserver::~SpriteObserver() {
}

// Load a single texture
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;
  stbi_uc* pixels = stbi_load(imageFilename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image!");
  }

  spdlog::debug("Sprite loaded: {0}, width={1}, height{2}. channels={3}", imageFilename, width, height, channels);

  return {std::shared_ptr<uint8_t[]>(pixels), (uint32_t)width, (uint32_t)height, (uint32_t)4};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(uint32_t gridWidth, uint32_t gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

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

std::unique_ptr<uint8_t[]> SpriteObserver::reset() const {
  auto ctx = device_->beginRender();

  render(ctx);

  auto width = grid_->getWidth() * tileSize_;
  auto height = grid_->getHeight() * tileSize_;

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles = {
      {{0, 0},
       {width, height}}};

  return device_->endRender(ctx, dirtyRectangles);
}

std::unique_ptr<uint8_t[]> SpriteObserver::update(int playerId) const {
  auto ctx = device_->beginRender();

  render(ctx);

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles;

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto l : updatedLocations) {
    VkOffset2D offset = {l.x * tileSize_, l.y * tileSize_};
    VkExtent2D extent = {tileSize_, tileSize_};

    dirtyRectangles.push_back({offset, extent});
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
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto offset = (float)tileSize_ / 2.0f;

  // have to get the objects in z buffer order so transparency effects work.
  // Order Independent Transparency is complicated and overkill here

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < width; y++) {
      GridLocation location{x, y};

      auto objects = grid_->getObjectsAt(location);

      // Have to use a reverse iterator
      for (auto objectIt = objects.rbegin(); objectIt != objects.rend(); objectIt++) {
        auto object = objectIt->second;

        float scale = (float)tileSize_;
        auto objectName = object->getObjectName();

        auto spriteName = getSpriteName(objectName, location);

        glm::vec3 color = {1.0, 1.0, 1.0};
        uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(spriteName);

        // Just a hack to keep depth between 0 and 1
        auto zCoord = (float)object->getZIdx() / 10.0;

        glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -zCoord};
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
        device_->drawSprite(ctx, spriteArrayLayer, model, color);
      }
    }
  }
}

}  // namespace griddy