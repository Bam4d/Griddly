#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"

namespace griddly {

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions) : VulkanGridObserver(grid, resourceConfig), blockDefinitions_(blockDefinitions) {
}

BlockObserver::~BlockObserver() {
}

ObserverType BlockObserver::getObserverType() const {
 return ObserverType::BLOCK_2D; 
}

void BlockObserver::init(ObserverConfig observerConfig) {
  VulkanGridObserver::init(observerConfig);

  device_->initRenderMode(vk::RenderMode::SHAPES);

  for (auto blockDef : blockDefinitions_) {
    auto objectName = blockDef.first;
    auto definition = blockDef.second;

    auto shapeBuffer = device_->getShapeBuffer(definition.shape);

    auto color = definition.color;
    glm::vec3 col = {color[0], color[1], color[2]};

    blockConfigs_.insert({objectName, {col, shapeBuffer, definition.scale, definition.outlineScale}});
  }

  resetRenderSurface();
}

void BlockObserver::renderLocation(vk::VulkanRenderContext& ctx, glm::ivec2 objectLocation, glm::ivec2 outputLocation, glm::ivec2 tileOffset, DiscreteOrientation orientation) const {
  auto objects = grid_->getObjectsAt(objectLocation);
  auto tileSize = observerConfig_.tileSize;

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

    auto shapeColor = glm::vec4(blockConfig.color, 1.0);
    glm::vec3 position = glm::vec3(tileOffset + outputLocation * tileSize, zCoord - 1.0);
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), {blockConfig.scale * tileSize.x, blockConfig.scale * tileSize.y, 1.0});
    auto orientedModel = glm::rotate(model, objectRotationRad, glm::vec3(0.0, 0.0, 1.0));


    if (observerConfig_.playerCount > 1 && objectPlayerId > 0) {
      auto playerId = observerConfig_.playerId;

      glm::vec4 outlineColor;

      if (playerId == objectPlayerId) {
        outlineColor = glm::vec4(0.0, 1.0, 0.0, 0.7);
      } else {
        outlineColor = globalObserverPlayerColors_[objectPlayerId-1];
      }

      device_->drawShapeWithOutline(ctx, blockConfig.shapeBuffer, orientedModel, shapeColor, outlineColor);
    } else {
      device_->drawShape(ctx, blockConfig.shapeBuffer, orientedModel, shapeColor);

    }

    
  }
}

}  // namespace griddly