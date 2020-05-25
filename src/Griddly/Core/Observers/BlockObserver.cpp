#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"

namespace griddly {

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, uint32_t tileSize, std::unordered_map<std::string, BlockDefinition> blockDefinitions, std::string resourcePath) : VulkanObserver(grid, tileSize, resourcePath), blockDefinitions_(blockDefinitions) {
}

BlockObserver::~BlockObserver() {
}

void BlockObserver::init(uint32_t gridWidth, uint32_t gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

  device_->initRenderMode(vk::RenderMode::SHAPES);

  for (auto blockDef : blockDefinitions_) {
    auto objectName = blockDef.first;
    auto definition = blockDef.second;

    auto shapeBuffer = device_->getShapeBuffer(definition.shape);

    auto color = definition.color;
    glm::vec3 col = {color[0], color[1], color[2]};

    blockConfigs_.insert({objectName, {col, shapeBuffer, definition.scale}});
  }
}

std::unique_ptr<uint8_t[]> BlockObserver::reset() const {
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

std::unique_ptr<uint8_t[]> BlockObserver::update(int playerId) const {
  auto ctx = device_->beginRender();

  render(ctx);
  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles;

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto l : updatedLocations) {
    VkOffset2D offset = {(int32_t)(l.x * tileSize_), (int32_t)(l.y * tileSize_)};
    VkExtent2D extent = {tileSize_, tileSize_};

    dirtyRectangles.push_back({offset, extent});
  }

  return device_->endRender(ctx, dirtyRectangles);
}

void BlockObserver::render(vk::VulkanRenderContext& ctx) const {
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto offset = (float)tileSize_ / 2.0f;

  for (uint32_t x = 0; x < width; x++) {
    for (uint32_t y = 0; y < width; y++) {
      GridLocation location{x, y};

      auto objects = grid_->getObjectsAt(location);

      // Have to use a reverse iterator
      for (auto objectIt = objects.begin(); objectIt != objects.end(); objectIt++) {
        auto object = objectIt->second;
        auto objectName = object->getObjectName();

        auto blockConfigIt = blockConfigs_.find(objectName);
        auto blockConfig = blockConfigIt->second;

        // Just a hack to keep depth between 0 and 1
        auto zCoord = (float)object->getZIdx() / 10.0;

        glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, zCoord - 1.0};
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(blockConfig.scale * tileSize_));
        device_->drawShape(ctx, blockConfig.shapeBuffer, model, blockConfig.color);
      }
    }
  }
}

}  // namespace griddly