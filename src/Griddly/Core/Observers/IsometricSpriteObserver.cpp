#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "IsometricSpriteObserver.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

IsometricSpriteObserver::IsometricSpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions, ShaderVariableConfig shaderVariableConfig)
    : SpriteObserver(grid, resourceConfig, spriteDefinitions, shaderVariableConfig) {
}

IsometricSpriteObserver::~IsometricSpriteObserver() {
}

ObserverType IsometricSpriteObserver::getObserverType() const {
  return ObserverType::ISOMETRIC;
}

void IsometricSpriteObserver::resetShape() {
  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = observerConfig_.tileSize;

  pixelWidth_ = (gridWidth_ + gridHeight_) * tileSize.x / 2;
  pixelHeight_ = (gridWidth_ + gridHeight_) * (observerConfig_.isoTileHeight / 2) + tileSize.y + observerConfig_.isoTileDepth;

  observationShape_ = {3, pixelWidth_, pixelHeight_};

  auto isoHeightRatio = static_cast<float>(observerConfig_.isoTileHeight) / static_cast<float>(tileSize.y);

  glm::vec2 midpoint(static_cast<float>(gridWidth_) / 2.0 - 0.5, static_cast<float>(gridHeight_) / 2.0 - 0.5);

  // Scale and shear for isometric locations
  isoTransform_ = isoTransform_ * glm::mat4({
                                      {0.5f, -0.5f * isoHeightRatio, 0, 0},
                                      {0.5f, 0.5f * isoHeightRatio, 0, 0},
                                      {0, 0, 1.0f, 0},
                                      {0, 1.0 - isoHeightRatio, 0, 1.0f},
                                  });

  // Translate back to center
  isoTransform_ = glm::translate(isoTransform_, glm::vec3(midpoint, 0.0));

  // Rotate 90 degrees to correct orientation
  isoTransform_ = glm::rotate(isoTransform_, glm::pi<float>() / 2.0f, glm::vec3(0.0, 0.0, 1.0));

  // Translate to origin
  isoTransform_ = glm::translate(isoTransform_, glm::vec3(-midpoint, 0.0));
}

std::vector<vk::ObjectDataSSBO> IsometricSpriteObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {
  auto isoGlobalModelMatrix = isoTransform_ * globalModelMatrix;

  std::vector<vk::ObjectDataSSBO> objectDataSSBOData;

  auto tileSize = getTileSize();

  auto backgroundTextureIndex = device_->getSpriteArrayLayer("_iso_background_");

  // Have to go through each location
  for (int x = observableGrid.left; x <= observableGrid.right; x++) {
    for (int y = observableGrid.bottom; y <= observableGrid.top; y++) {
      glm::vec2 location{x, y};
      auto objectAtLocation = grid_->getObjectsAt(location);

      // Translate the locations with respect to global transform
      glm::vec4 renderLocation = isoGlobalModelMatrix * glm::vec4(location, 0.0, 1.0);

      if (objectAtLocation.size() == 0) {
        vk::ObjectDataSSBO backgroundTiling;
        backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
        backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));
        backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(0.5, 0.5, 0.0));
        backgroundTiling.zIdx = -1;
        backgroundTiling.textureIndex = backgroundTextureIndex;
        objectDataSSBOData.push_back(backgroundTiling);
      }

      for (auto objectIt = objectAtLocation.begin(); objectIt != objectAtLocation.end(); ++objectIt) {
        vk::ObjectDataSSBO objectData;

        auto object = objectIt->second;

        auto objectName = object->getObjectName();
        auto tileName = object->getObjectRenderTileName();
        auto objectPlayerId = object->getPlayerId();
        auto zIdx = object->getZIdx();

        auto spriteDefinition = spriteDefinitions_.at(tileName);
        auto tileOffset = glm::vec2(spriteDefinition.offset.x / tileSize.x, spriteDefinition.offset.y / tileSize.y);
        auto tilingMode = spriteDefinition.tilingMode;
        auto isIsoFloor = tilingMode == TilingMode::ISO_FLOOR;

        spdlog::debug("Updating object {0} at location [{1},{2}]", objectName, location.x, location.y);

        if (objectIt == objectAtLocation.begin() && !isIsoFloor) {
          vk::ObjectDataSSBO backgroundTiling;
          backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
          backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));
          backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(0.5, 0.5, 0.0));
          backgroundTiling.zIdx = -1;
          backgroundTiling.textureIndex = backgroundTextureIndex;
          objectDataSSBOData.push_back(backgroundTiling);
        }

        // Translate
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));  // Observer offsets
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(tileOffset, 0.0));
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)

        // Scale the objects based on their scales
        auto scale = spriteDefinition.scale;
        objectData.modelMatrix = glm::scale(objectData.modelMatrix, glm::vec3(scale, scale, 1.0));

        auto spriteName = getSpriteName(objectName, tileName, location, globalOrientation.getDirection());
        objectData.textureIndex = device_->getSpriteArrayLayer(spriteName);
        objectData.playerId = objectPlayerId;
        objectData.zIdx = zIdx;

        objectDataSSBOData.push_back(objectData);
      }
    }
  }

  // Sort by z-index and y-index, so we render things on top of each other in the right order
  std::sort(objectDataSSBOData.begin(), objectDataSSBOData.end(),
            [this](const vk::ObjectDataSSBO& a, const vk::ObjectDataSSBO& b) -> bool {
              if (a.modelMatrix[3][1] == b.modelMatrix[3][1]) {
                return a.zIdx < b.zIdx;
              } else {
                return a.modelMatrix[3][1] < b.modelMatrix[3][1];
              }
            });

  return objectDataSSBOData;
}

}  // namespace griddly
