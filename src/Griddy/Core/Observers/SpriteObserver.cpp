#include "SpriteObserver.hpp"

#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "../Objects/Terrain/Minerals.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddy {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, uint spriteSize) : VulkanObserver(grid, spriteSize) {
}

SpriteObserver::~SpriteObserver() {
}

// Load a single texture
SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  uint width, height, channels;
  stbi_uc* pixels = stbi_load(imageFilename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image!");
  }

  return {std::shared_ptr<uint8_t[]>(pixels), width, height, channels};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(uint gridWidth, uint gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

  device_->preloadSprites({"test" : {loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_celestial_d1.png")}})

      device_->initRenderMode(vk::RenderMode::SPRITES);
}

std::unique_ptr<uint8_t[]> SpriteObserver::observe(int playerId) {
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto ctx = device_->beginRender();

  auto offset = (float)tileSize_ / 2.0f;

  auto objects = grid_->getObjects();

  for (const auto& object : objects) {
    float scale = (float)tileSize_;
    auto location = object->getLocation();
    auto objectType = object->getObjectType();

    glm::vec3 color = {};
    uint32_t spriteArrayLayer = device_->getSpriteArrayLayer("test");
    switch (objectType) {
      case HARVESTER:
        color = {0.6, 0.2, 0.2};
        scale *= 0.7;
        break;
      case MINERALS: {
        color = {0.0, 1.0, 0.0};
        auto minerals = std::dynamic_pointer_cast<Minerals>(object);
        scale *= ((float)minerals->getValue() / minerals->getMaxValue());
      } break;
      case PUSHER:
        color = {0.2, 0.2, 0.6};
        scale *= 0.8;
        break;
      case PUNCHER:
        color = {0.2, 0.6, 0.6};
        scale *= 0.8;
        break;
      case FIXED_WALL:
        color = {0.5, 0.5, 0.5};
        break;
      case PUSHABLE_WALL:
        color = {0.8, 0.8, 0.8};
        break;
    }

    glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -1.0f};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
    device_->drawSprite(ctx, spriteArrayLayer, model, color);
  }

  //   for(int x = 0; x<width; x++) {
  //     for(int y = 0; y<height; y++) {

  //       glm::vec3 position = {offset+x*tileSize_, offset+y*tileSize_, -1.0f};
  //       glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
  //       device_->drawShape(ctx, square, model, color);
  //     }
  //   }

  return device_->endRender(ctx);
}

}  // namespace griddy