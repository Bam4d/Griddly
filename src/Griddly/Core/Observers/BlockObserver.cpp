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

    blockConfigs_.insert({objectName, {col, shapeBuffer, definition.scale, definition.outlineScale}});
  }

}

void BlockObserver::renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, float tileOffset, DiscreteOrientation orientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);
  auto scale = vulkanObserverConfig_.tileSize;

  for (auto objectIt : objects) {
    auto object = objectIt.second;
    auto tileName = object->getObjectRenderTileName();
    float objectRotationRad;
    
    if (object == avatarObject_ && observerConfig_.rotateWithAvatar) {
      objectRotationRad = 0.0;
    } else {
      objectRotationRad = object->getObjectOrientation().getAngleRadians();
    }

    auto blockConfigIt = blockConfigs_.find(tileName);
    auto blockConfig = blockConfigIt->second;

    // Just a hack to keep depth between 0 and 1
    auto zCoord = (float)object->getZIdx() / 10.0;

    auto objectPlayerId = object->getPlayerId();

    if (observerConfig_.playerCount > 1 && objectPlayerId > 0) {
      auto playerId = observerConfig_.playerId;

      glm::vec4 outlineColor;

      if (playerId == objectPlayerId) {
        outlineColor = glm::vec4(0.0, 1.0, 0.0, 0.7);
      } else {
        outlineColor = globalObserverPlayerColors_[objectPlayerId-1];
      }

      glm::vec3 position = {tileOffset + outputLocation.x * scale, tileOffset + outputLocation.y * scale, zCoord - 1.0};
      glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(blockConfig.scale * scale));
      auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
      device_->drawShapeOutline(ctx, blockConfig.shapeBuffer, model, blockConfig.outlineScale, outlineColor);
    }

    glm::vec3 position = {tileOffset + outputLocation.x * scale, tileOffset + outputLocation.y * scale, zCoord - 1.0};
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(blockConfig.scale * scale));
    auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));
    device_->drawShape(ctx, blockConfig.shapeBuffer, orientedModel, glm::vec4(blockConfig.color, 1.0));
  }
}

}  // namespace griddly