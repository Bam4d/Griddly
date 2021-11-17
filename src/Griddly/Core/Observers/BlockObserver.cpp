#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"

namespace griddly {

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions, ShaderVariableConfig shaderVariableConfig) : VulkanGridObserver(grid, resourceConfig, shaderVariableConfig), blockDefinitions_(blockDefinitions) {
}

BlockObserver::~BlockObserver() {
}

ObserverType BlockObserver::getObserverType() const {
  return ObserverType::BLOCK_2D;
}

void BlockObserver::lazyInit() {
  VulkanObserver::lazyInit();

  device_->initRenderMode(vk::RenderMode::SHAPES);

  shapeBuffers_.push_back(device_->getShapeBuffer("square"));
  shapeBuffers_.push_back(device_->getShapeBuffer("triangle"));

  for (auto blockDef : blockDefinitions_) {
    auto objectName = blockDef.first;
    auto definition = blockDef.second;

    auto shapeBufferId = 0;

    if (definition.shape == "square") {
      shapeBufferId = 0;
    } else if (definition.shape == "triangle") {
      shapeBufferId = 1;
    }

    auto color = definition.color;
    glm::vec3 col = {color[0], color[1], color[2]};

    blockConfigs_.insert({objectName, {col, shapeBufferId, definition.scale, definition.outlineScale}});
  }
}

std::vector<vk::ObjectDataSSBO> BlockObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {
  std::vector<vk::ObjectDataSSBO> objectDataSSBOData;

  auto objects = grid_->getObjects();

  for (auto object : objects) {
    vk::ObjectDataSSBO objectData;
    auto location = object->getLocation();

    // Check we are within the boundary of the render grid otherwise don't add the object
    if (location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top) {
      continue;
    }

    auto objectOrientation = object->getObjectOrientation();
    auto objectName = object->getObjectName();
    auto tileName = object->getObjectRenderTileName();
    auto objectPlayerId = object->getPlayerId();
    auto zIdx = object->getZIdx();

    spdlog::debug("Updating object {0} at location [{1},{2}]", objectName, location.x, location.y);

    auto blockConfig = blockConfigs_.at(tileName);

    

    // Translate the locations with respect to global transform
    glm::vec4 renderLocation = globalModelMatrix * glm::vec4(location, 0.0, 1.0);

    // Translate
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));  // Observer offsets
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));                                                  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)

    // Rotate the objects that should be rotated
    if (!(object == avatarObject_ && observerConfig_.rotateWithAvatar)) {
      auto objectAngleRadians = objectOrientation.getAngleRadians() - globalOrientation.getAngleRadians();
      objectData.modelMatrix = glm::rotate(objectData.modelMatrix, objectAngleRadians, glm::vec3(0.0, 0.0, 1.0));
    }

    // Scale the objects based on their scales
    auto scale = blockConfig.scale;
    objectData.modelMatrix = glm::scale(objectData.modelMatrix, glm::vec3(scale, scale, 1.0));

    objectData.color = glm::vec4(blockConfig.color, 1.0);
    objectData.playerId = objectPlayerId;
    objectData.zIdx = zIdx;
    objectData.textureIndex = blockConfig.shapeBufferId;

    objectDataSSBOData.push_back(objectData);
  }

  // Sort by z-index, so we render things on top of each other in the right order
  std::sort(objectDataSSBOData.begin(), objectDataSSBOData.end(),
            [this](const vk::ObjectDataSSBO& a, const vk::ObjectDataSSBO& b) -> bool {
              return a.zIdx < b.zIdx;
            });

  return objectDataSSBOData;
}

void BlockObserver::updateCommandBuffer(std::vector<vk::ObjectDataSSBO> objectData) {
  for (int i = 0; i < objectData.size(); i++) {
    auto objectDataSSBO = objectData[i];
    auto shapeBuffer = shapeBuffers_[objectDataSSBO.textureIndex];
    device_->updateObjectPushConstants(i, shapeBuffer);
  }
}

}  // namespace griddly