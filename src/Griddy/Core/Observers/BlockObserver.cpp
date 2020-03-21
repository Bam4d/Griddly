#include "BlockObserver.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddy {

BlockObserver::BlockObserver(uint32_t tileSize) : VulkanObserver(tileSize) {
}

BlockObserver::~BlockObserver() {
}

std::unique_ptr<uint8_t[]> BlockObserver::observe(int playerId, std::shared_ptr<Grid> grid) {
  auto width = grid->getWidth();
  auto height = grid->getHeight();

  auto ctx = device_->beginRender();

  float scale = (float)tileSize_;
  auto offset = (float)tileSize_ / 2.0f;

  auto square = device_->getShapeBuffer("square");
  auto triangle = device_->getShapeBuffer("triangle");

  auto objects = grid->getObjects();

  for (const auto& object : objects) {
    auto location = object->getLocation();
    auto objectType = object->getObjectType();

    vk::ShapeBuffer* shapeBuffer;
    glm::vec3 color = {};
    switch (objectType) {
      case HARVESTER:
        color = {0.6, 0.2, 0.2};
        shapeBuffer = &square;
        break;
      case MINERALS:
        color = {0.0, 1.0, 0.0};
        shapeBuffer = &triangle;
        break;
    }

    glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -1.0f};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(scale));
    device_->drawShape(ctx, *shapeBuffer, model, color);
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