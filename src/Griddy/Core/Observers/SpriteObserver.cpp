#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
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

  spdlog::debug("Sprite loaded: {0}, width={1}, height{2}. channels={3}", imageFilename, width, height, channels);

  return {std::shared_ptr<uint8_t[]>(pixels), width, height, 4};
}

/** loads the sprites needed for rendering **/
void SpriteObserver::init(uint gridWidth, uint gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

  std::unordered_map<std::string, vk::SpriteData> spriteData = {
      {"base", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img324.png")},
      {"harvester", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png")},
      {"puncher", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png")},
      {"pusher", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png")},
      {"minerals", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png")},

      {"fixed_wall_a", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img40.png")},
      {"fixed_wall_b", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img33.png")},

      {"pushable_wall", loadImage("resources/images/oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img282.png")},
  };

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
    auto objectType = object->getObjectType();

    glm::vec3 color = {1.0, 1.0, 1.0};
    uint32_t spriteArrayLayer;
    switch (objectType) {
      case BASE:
        spriteArrayLayer = device_->getSpriteArrayLayer("base");
        break;
      case HARVESTER:
        spriteArrayLayer = device_->getSpriteArrayLayer("harvester");
        break;
      case MINERALS: {
        auto minerals = std::dynamic_pointer_cast<Minerals>(object);
        scale *= ((float)minerals->getValue() / minerals->getMaxValue());
        spriteArrayLayer = device_->getSpriteArrayLayer("minerals");
      } break;
      case PUSHER:
        spriteArrayLayer = device_->getSpriteArrayLayer("pusher");
        break;
      case PUNCHER:
        spriteArrayLayer = device_->getSpriteArrayLayer("puncher");
        break;
      case FIXED_WALL: {
        // If there is a wall below this one then we display a different image
        auto objectBelow = grid_->getObject({location.x, location.y + 1});
        if (objectBelow != nullptr && objectBelow->getObjectType() == FIXED_WALL) {
          spriteArrayLayer = device_->getSpriteArrayLayer("fixed_wall_a");
        } else {
          spriteArrayLayer = device_->getSpriteArrayLayer("fixed_wall_b");
        }
        break;
      }
      case PUSHABLE_WALL:
        spriteArrayLayer = device_->getSpriteArrayLayer("pushable_wall");
        break;
    }

    glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -1.0f};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
    device_->drawSprite(ctx, spriteArrayLayer, model, color);
  }
}

}  // namespace griddy