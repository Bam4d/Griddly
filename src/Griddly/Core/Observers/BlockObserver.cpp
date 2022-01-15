#include "BlockObserver.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"

namespace griddly {

const std::unordered_map<std::string, SpriteDefinition> BlockObserver::blockSpriteDefinitions_ = {
  {"circle", {{"block_shapes/circle.png"}}},
  {"triangle", {{"block_shapes/triangle.png"}}},
  {"square", {{"block_shapes/square.png"}}},
  {"pentagon", {{"block_shapes/pentagon.png"}}},
  {"hexagon", {{"block_shapes/hexagon.png"}}},
};

BlockObserver::BlockObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, BlockDefinition> blockDefinitions, ShaderVariableConfig shaderVariableConfig)
    : SpriteObserver(grid, resourceConfig, blockSpriteDefinitions_, shaderVariableConfig), blockDefinitions_(blockDefinitions) {
    
}

BlockObserver::~BlockObserver() {
}

ObserverType BlockObserver::getObserverType() const {
  return ObserverType::BLOCK_2D;
}

std::vector<vk::ObjectSSBOs> BlockObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {
  std::vector<vk::ObjectSSBOs> objectSSBOData{};

  const auto& objects = grid_->getObjects();
  const auto objectIds = grid_->getObjectIds();

  for (auto& object : objects) {
    vk::ObjectDataSSBO objectData;
    std::vector<vk::ObjectVariableSSBO> objectVariableData;
    auto location = object->getLocation();

    // Check we are within the boundary of the render grid otherwise don't add the object
    if (location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top) {
      continue;
    }

    auto objectOrientation = object->getObjectOrientation();
    auto objectName = object->getObjectName();
    auto tileName = object->getObjectRenderTileName();
    auto objectPlayerId = object->getPlayerId();
    auto objectTypeId = objectIds.at(objectName);
    auto zIdx = object->getZIdx();

    spdlog::trace("Updating object {0} at location [{1},{2}]", objectName, location.x, location.y);

    const auto& blockDefinition = blockDefinitions_.at(tileName);

    // Translate the locations with respect to global transform
    glm::vec4 renderLocation = globalModelMatrix * glm::vec4(location, 0.0, 1.0);

    // Translate
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)

    // Rotate the objects that should be rotated
    if (!(object == avatarObject_ && observerConfig_.rotateWithAvatar)) {
      auto objectAngleRadians = objectOrientation.getAngleRadians() - globalOrientation.getAngleRadians();
      objectData.modelMatrix = glm::rotate(objectData.modelMatrix, objectAngleRadians, glm::vec3(0.0, 0.0, 1.0));
    }

    // Scale the objects based on their scales
    auto scale = blockDefinition.scale;
    objectData.modelMatrix = glm::scale(objectData.modelMatrix, glm::vec3(scale, scale, 1.0));

    objectData.color = glm::vec4(blockDefinition.color[0], blockDefinition.color[1], blockDefinition.color[2], 1.0);
    objectData.playerId = objectPlayerId;
    objectData.textureIndex = device_->getSpriteArrayLayer(blockDefinition.shape);
    objectData.objectTypeId = objectTypeId;
    objectData.zIdx = zIdx;

    for(auto variableValue : getExposedVariableValues(object)) {
      objectVariableData.push_back({variableValue});
    }

    objectSSBOData.push_back({objectData, objectVariableData});
  }

  // Sort by z-index, so we render things on top of each other in the right order
  std::sort(objectSSBOData.begin(), objectSSBOData.end(),
            [this](const vk::ObjectSSBOs& a, const vk::ObjectSSBOs& b) -> bool {
              return a.objectData.zIdx < b.objectData.zIdx;
            });

  return objectSSBOData;
}

}  // namespace griddly