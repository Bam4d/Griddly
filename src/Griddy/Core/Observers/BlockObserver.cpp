#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"


namespace griddy {

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, uint32_t tileSize, std::unordered_map<std::string, BlockDefinition> blockDefinitions) : VulkanObserver(grid, tileSize), blockDefinitions_(blockDefinitions) {
}

BlockObserver::~BlockObserver() {
}

void BlockObserver::init(uint gridWidth, uint gridHeight) {
  VulkanObserver::init(gridWidth, gridHeight);

  device_->initRenderMode(vk::RenderMode::SHAPES);

  for(auto blockDef : blockDefinitions_) {
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
    VkOffset2D offset = {l.x * tileSize_, l.y * tileSize_};
    VkExtent2D extent = {tileSize_, tileSize_};

    dirtyRectangles.push_back({offset, extent});
  }

  return device_->endRender(ctx, dirtyRectangles);
}

void BlockObserver::render(vk::VulkanRenderContext& ctx) const {
  auto width = grid_->getWidth();
  auto height = grid_->getHeight();

  auto offset = (float)tileSize_ / 2.0f;


  auto objects = grid_->getObjects();

  for (const auto& object : objects) {
    auto location = object->getLocation();
    auto objectName = object->getObjectName();

    auto blockConfigIt = blockConfigs_.find(objectName);
    auto blockConfig = blockConfigIt->second;

    glm::vec3 position = {offset + location.x * tileSize_, offset + location.y * tileSize_, -1.0f};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(blockConfig.scale * tileSize_));
    device_->drawShape(ctx, blockConfig.shapeBuffer, model, blockConfig.color);
  }
}

}  // namespace griddy