#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddy {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, uint32_t spriteSize, std::unordered_map<std::string, std::string> spriteDesciptions) : VulkanObserver(grid, spriteSize), spriteDesciptions_(spriteDesciptions) {
  
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
  for (auto spriteDescription: spriteDesciptions_) {
    auto spriteName = spriteDescription.first;
    auto spriteFilename = spriteDescription.second;
    spriteData.insert({spriteName, loadImage(spriteFilename)});
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

void SpriteObserver::render(vk::VulkanRenderContext& ctx) const {
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto offset = (float)tileSize_ / 2.0f;

  auto objects = grid_->getObjects();

  for (const auto& object : objects) {
    float scale = (float)tileSize_;
    auto location = object->getLocation();
    auto objectName = object->getObjectName();

    glm::vec3 color = {1.0, 1.0, 1.0};
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer(objectName);

    glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -1.0f};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
    device_->drawSprite(ctx, spriteArrayLayer, model, color);
  }
}

}  // namespace griddy