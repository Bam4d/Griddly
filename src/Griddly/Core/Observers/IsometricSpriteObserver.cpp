#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "IsometricSpriteObserver.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

IsometricSpriteObserver::IsometricSpriteObserver(std::shared_ptr<Grid> grid, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions)
    : SpriteObserver(grid, spriteDefinitions) {
}

ObserverType IsometricSpriteObserver::getObserverType() const {
  return ObserverType::ISOMETRIC;
}

void IsometricSpriteObserver::init(IsometricSpriteObserverConfig& config) {
  SpriteObserver::init(config);
  config_ = config;
}

const IsometricSpriteObserverConfig& IsometricSpriteObserver::getConfig() const {
  return config_;
}

void IsometricSpriteObserver::resetShape() {
  const auto& config = getConfig();

  gridWidth_ = config.overrideGridWidth > 0 ? config.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config.overrideGridHeight > 0 ? config.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = config.tileSize;

  pixelWidth_ = (gridWidth_ + gridHeight_) * tileSize.x / 2;
  pixelHeight_ = (gridWidth_ + gridHeight_) * (config.isoTileHeight / 2) + tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};

  isoHeightRatio_ = static_cast<float>(config.isoTileHeight) / static_cast<float>(tileSize.y);

  // Scale and shear for isometric locations
  isoTransform_ = glm::mat4({{0.5, 0.5 * isoHeightRatio_, 0, 0},
                             {-0.5, 0.5 * isoHeightRatio_, 0, 0},
                             {0, 0, 1, 0},
                             {0, 0, 0, 1}});
}

glm::mat4 IsometricSpriteObserver::getGlobalModelMatrix() {
  glm::mat4 globalModelMatrix(1);

  const auto& config = getConfig();

  globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(config.gridXOffset, config.gridYOffset, 0.0));

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();

    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(gridWidth_ / 2.0 - 0.5, gridHeight_ / 2.0 - 0.5, 0.0));

    if (config.rotateWithAvatar) {
      globalModelMatrix = glm::rotate(globalModelMatrix, -avatarObject_->getObjectOrientation().getAngleRadians(), glm::vec3(0.0, 0.0, 1.0));
    }

    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(-avatarLocation, 0.0));
  }

  return isoTransform_ * globalModelMatrix;
}

glm::mat4 IsometricSpriteObserver::getViewMatrix() {
  glm::mat4 viewMatrix(1);
  const auto& config = getConfig();
  viewMatrix = glm::scale(viewMatrix, glm::vec3(config.tileSize, 1.0));                  //scale by tile size
  viewMatrix = glm::translate(viewMatrix, glm::vec3((gridHeight_ - 1) / 2.0, 0.0, 0.0));  // iso offset for X
  viewMatrix = glm::translate(viewMatrix, glm::vec3(0.5, 0.5, 0.0));                      // vertex offset
  return viewMatrix;
}

void IsometricSpriteObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {
  auto tileSize = getTileSize();
  const auto& objectIds = grid_->getObjectIds();

  auto backgroundTextureIndex = device_->getSpriteArrayLayer("_iso_background_");

  // Have to go through each location
  for (int x = observableGrid.left; x <= observableGrid.right; x++) {
    for (int y = observableGrid.bottom; y <= observableGrid.top; y++) {
      glm::vec2 location{x, y};
      const auto& objectAtLocation = grid_->getObjectsAt(location);

      // Translate the locations with respect to global transform
      glm::vec4 renderLocation = globalModelMatrix * glm::vec4(location, 0.0, 1.0);

      if (objectAtLocation.size() == 0) {
        vk::ObjectDataSSBO backgroundTiling{};
        backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
        backgroundTiling.zIdx = -1;
        backgroundTiling.textureIndex = backgroundTextureIndex;
        frameSSBOData_.objectSSBOData.push_back({backgroundTiling});
      }

      for (auto objectIt = objectAtLocation.begin(); objectIt != objectAtLocation.end(); ++objectIt) {
        vk::ObjectDataSSBO objectData{};
        std::vector<vk::ObjectVariableSSBO> objectVariableData{};

        auto object = objectIt->second;

        const auto& objectName = object->getObjectName();
        const auto& tileName = object->getObjectRenderTileName();
        auto objectPlayerId = object->getPlayerId();
        auto objectTypeId = objectIds.at(objectName);
        auto zIdx = object->getZIdx();

        if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
          throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
        }

        const auto& spriteDefinition = spriteDefinitions_.at(tileName);
        auto tileOffset = glm::vec2(spriteDefinition.offset.x / tileSize.x, spriteDefinition.offset.y / tileSize.y);
        auto tilingMode = spriteDefinition.tilingMode;
        auto isIsoFloor = tilingMode == TilingMode::ISO_FLOOR;

        if (isIsoFloor && zIdx == 0) {
          zIdx = -1;
        }

        spdlog::debug("Updating object {0} at location [{1},{2}]", objectName, location.x, location.y);

        if (objectIt == objectAtLocation.begin() && !isIsoFloor) {
          vk::ObjectDataSSBO backgroundTiling{};
          backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
          backgroundTiling.zIdx = -1;
          backgroundTiling.textureIndex = backgroundTextureIndex;
          frameSSBOData_.objectSSBOData.push_back({backgroundTiling});
        }

        // Translate
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
        objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(tileOffset, 0.0));

        // Scale the objects based on their scales
        auto scale = spriteDefinition.scale;
        objectData.modelMatrix = glm::scale(objectData.modelMatrix, glm::vec3(scale, scale, 1.0));

        auto spriteName = getSpriteName(objectName, tileName, location, globalOrientation.getDirection());
        objectData.textureIndex = device_->getSpriteArrayLayer(spriteName);
        objectData.playerId = objectPlayerId;
        objectData.zIdx = zIdx;
        objectData.objectTypeId = objectTypeId;

        for (auto variableValue : getExposedVariableValues(object)) {
          objectVariableData.push_back({variableValue});
        }

        frameSSBOData_.objectSSBOData.push_back({objectData, objectVariableData});
      }
    }
  }

  // Sort by z-index and y-index, so we render things on top of each other in the right order
  std::sort(frameSSBOData_.objectSSBOData.begin(), frameSSBOData_.objectSSBOData.end(),
            [this](const vk::ObjectSSBOs& a, const vk::ObjectSSBOs& b) -> bool {
              if (a.objectData.zIdx == b.objectData.zIdx) {
                return a.objectData.modelMatrix[3][1] < b.objectData.modelMatrix[3][1];
              } else {
                return a.objectData.zIdx < b.objectData.zIdx;
              }
            });
}

}  // namespace griddly
