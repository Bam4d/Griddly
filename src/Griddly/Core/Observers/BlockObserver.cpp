#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"

namespace griddly {

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions) : VulkanGridObserver(grid, vulkanObserverConfig), blockDefinitions_(blockDefinitions) {
}

BlockObserver::~BlockObserver() {
}

void BlockObserver::init(ObserverConfig observerConfig) {
  VulkanObserver::init(observerConfig);

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

void BlockObserver::renderLocation(vk::VulkanRenderContext& ctx, GridLocation objectLocation, GridLocation outputLocation, float scale, float tileOffset, Direction orientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);

    // Have to use a reverse iterator
    for (auto objectIt = objects.begin(); objectIt != objects.end(); objectIt++) {
      auto object = objectIt->second;
      auto objectName = object->getObjectName();
      auto objectRotationRad = getObjectRotation(object);

      auto blockConfigIt = blockConfigs_.find(objectName);
      auto blockConfig = blockConfigIt->second;

      // Just a hack to keep depth between 0 and 1
      auto zCoord = (float)object->getZIdx() / 10.0;

      glm::vec3 position = {tileOffset + outputLocation.x * scale, tileOffset + outputLocation.y * scale, zCoord - 1.0};
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(blockConfig.scale * scale));
      auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
      device_->drawShape(ctx, blockConfig.shapeBuffer, orientedModel, blockConfig.color);
    }
}

}  // namespace griddly