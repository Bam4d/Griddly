#include "SpriteObserver.hpp"

#define STB_IMAGE_IMPLEMENTATION
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
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;
  stbi_uc* pixels = stbi_load(imageFilename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image!");
  }

  return {std::shared_ptr<uint8_t[]>(pixels), width, height, channels};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(uint gridWidth, uint gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

  std::unordered_map<std::string, vk::SpriteData> spriteData = {
      {"harvester", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png")},
      {"puncher", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png")},
      {"pusher", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png")},
      {"minerals", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png")},
      {"fixed_wall", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_wall_lab_v_a.png")},
      {"pushable_wall", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_barrel.png")},
  };

  device_->preloadSprites(spriteData);

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
    uint32_t spriteArrayLayer;
    switch (objectType) {
      case HARVESTER:
        color = {0.6, 0.2, 0.2};
        scale *= 0.7;
        spriteArrayLayer = device_->getSpriteArrayLayer("harvester");
        break;
      case MINERALS: {
        color = {0.0, 1.0, 0.0};
        auto minerals = std::dynamic_pointer_cast<Minerals>(object);
        scale *= ((float)minerals->getValue() / minerals->getMaxValue());
        spriteArrayLayer = device_->getSpriteArrayLayer("minerals");
      } break;
      case PUSHER:
        color = {0.2, 0.2, 0.6};
        scale *= 0.8;
        spriteArrayLayer = device_->getSpriteArrayLayer("pusher");
        break;
      case PUNCHER:
        color = {0.2, 0.6, 0.6};
        scale *= 0.8;
        spriteArrayLayer = device_->getSpriteArrayLayer("puncher");
        break;
      case FIXED_WALL:
        color = {0.5, 0.5, 0.5};
        spriteArrayLayer = device_->getSpriteArrayLayer("fixed_wall");
        break;
      case PUSHABLE_WALL:
        color = {0.8, 0.8, 0.8};
        spriteArrayLayer = device_->getSpriteArrayLayer("pushable_wall");
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