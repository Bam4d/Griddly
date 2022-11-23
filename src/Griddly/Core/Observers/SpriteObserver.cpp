#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include <glm/glm.hpp>
#include <utility>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, SpriteObserverConfig& config) : VulkanGridObserver(std::move(grid), config) {
  config_ = config;
  spriteDefinitions_ = config_.spriteDefinitions;
}

ObserverType SpriteObserver::getObserverType() const {
  return ObserverType::SPRITE_2D;
}

void SpriteObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  VulkanGridObserver::init(playerObservers);
}

// Load a single texture
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;

  std::string absoluteFilePath = config_.resourceConfig.imagePath + "/" + imageFilename;
  spdlog::debug("Loading Sprite {0}", absoluteFilePath);
  stbi_uc* pixels = stbi_load(absoluteFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error(fmt::format("Failed to load texture image {0}.", absoluteFilePath));
  }

  int outputWidth = config_.tileSize.x;
  int outputHeight = config_.tileSize.y;

  auto* resizedPixels = (stbi_uc*)malloc(outputWidth * outputHeight * 4);

  auto res = stbir_resize_uint8_generic(pixels, width, height, 0,
                                        resizedPixels, outputWidth, outputHeight, 0, 4,
                                        3,
                                        0,
                                        STBIR_EDGE_CLAMP,
                                        STBIR_FILTER_CATMULLROM,
                                        STBIR_COLORSPACE_LINEAR,
                                        nullptr);

  free(pixels);

  if (!res) {
    throw std::runtime_error("Failed to load texture image.");
  }

  spdlog::debug("Sprite loaded: {0}, width={1}, height={2}. channels={3}", absoluteFilePath, width, height, channels);

  std::unique_ptr<uint8_t[]> spriteData(resizedPixels);

  return {std::move(spriteData), (uint32_t)outputWidth, (uint32_t)outputHeight, (uint32_t)4};
}

void SpriteObserver::lazyInit() {
  VulkanObserver::lazyInit();

  std::map<std::string, vk::SpriteData> spriteData;
  for (auto spriteDefinitionIt : spriteDefinitions_) {
    auto spriteName = spriteDefinitionIt.first;
    auto spriteDefinition = spriteDefinitionIt.second;
    auto spriteImages = spriteDefinition.images;

    spdlog::debug("Loading sprite definition {0}", spriteName);

    if (spriteDefinition.tilingMode == TilingMode::WALL_2 || spriteDefinition.tilingMode == TilingMode::WALL_16) {
      if (spriteDefinition.tilingMode == TilingMode::WALL_2 && spriteImages.size() != 2 || spriteDefinition.tilingMode == TilingMode::WALL_16 && spriteImages.size() != 16) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectively. {0} images were supplied", spriteImages.size()));
      }

      for (int s = 0; s < spriteImages.size(); s++) {
        auto spriteNameAndIdx = spriteName + std::to_string(s);
        spdlog::debug("Loading sprite {0} image id {1}. Image: {2}", spriteName, spriteNameAndIdx, spriteDefinition.images[s]);
        spriteData.insert({spriteNameAndIdx, loadImage(spriteDefinition.images[s])});
      }
    } else {
      spdlog::debug("Loading sprite {0} image id {1}. Image: {2}", spriteName, 0, spriteDefinition.images[0]);
      spriteData.insert({spriteName, loadImage(spriteDefinition.images[0])});
    }
  }

  device_->preloadSprites(spriteData);
}

std::string SpriteObserver::getSpriteName(const std::string& objectName, const std::string& tileName, const glm::ivec2& location, Direction orientation) const {
  if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
    throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
  }

  auto& tilingMode = spriteDefinitions_.at(tileName).tilingMode;

  if (tilingMode == TilingMode::WALL_2) {
    auto objectDown = grid_->getObject({location.x, location.y + 1});
    int idx = 0;
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 1;
    }

    return tileName + std::to_string(idx);

  } else if (tilingMode == TilingMode::WALL_16) {
    std::shared_ptr<Object> objectLeft, objectRight, objectUp, objectDown;
    switch (orientation) {
      case Direction::NONE:
      case Direction::UP:
        objectLeft = grid_->getObject({location.x - 1, location.y});
        objectRight = grid_->getObject({location.x + 1, location.y});
        objectUp = grid_->getObject({location.x, location.y - 1});
        objectDown = grid_->getObject({location.x, location.y + 1});
        break;
      case Direction::DOWN:
        objectLeft = grid_->getObject({location.x + 1, location.y});
        objectRight = grid_->getObject({location.x - 1, location.y});
        objectUp = grid_->getObject({location.x, location.y + 1});
        objectDown = grid_->getObject({location.x, location.y - 1});
        break;
      case Direction::LEFT:
        objectLeft = grid_->getObject({location.x, location.y + 1});
        objectRight = grid_->getObject({location.x, location.y - 1});
        objectUp = grid_->getObject({location.x - 1, location.y});
        objectDown = grid_->getObject({location.x + 1, location.y});
        break;
      case Direction::RIGHT:
        objectLeft = grid_->getObject({location.x, location.y - 1});
        objectRight = grid_->getObject({location.x, location.y + 1});
        objectUp = grid_->getObject({location.x + 1, location.y});
        objectDown = grid_->getObject({location.x - 1, location.y});
        break;
      default:
        objectLeft = grid_->getObject({location.x - 1, location.y});
        objectRight = grid_->getObject({location.x + 1, location.y});
        objectUp = grid_->getObject({location.x, location.y - 1});
        objectDown = grid_->getObject({location.x, location.y + 1});
        break;
    }

    int idx = 0;
    if (objectLeft != nullptr && objectLeft->getObjectName() == objectName) {
      idx += 1;
    }
    if (objectRight != nullptr && objectRight->getObjectName() == objectName) {
      idx += 2;
    }
    if (objectUp != nullptr && objectUp->getObjectName() == objectName) {
      idx += 4;
    }
    if (objectDown != nullptr && objectDown->getObjectName() == objectName) {
      idx += 8;
    }

    return tileName + std::to_string(idx);
  }

  return tileName;
}

void SpriteObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {

  int32_t backgroundTileIndex = device_->getSpriteArrayLayer("_background_");
  if (backgroundTileIndex != -1) {
    vk::ObjectDataSSBO backgroundTiling;
    backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(gridWidth_ / 2.0 - config_.gridXOffset, gridHeight_ / 2.0 - config_.gridYOffset, 0.0));
    backgroundTiling.modelMatrix = glm::scale(backgroundTiling.modelMatrix, glm::vec3(gridWidth_, gridHeight_, 1.0));
    backgroundTiling.gridPosition = {0, 0, -10, 0};
    backgroundTiling.textureMultiply = {gridWidth_, gridHeight_};
    backgroundTiling.textureIndex = backgroundTileIndex;
    frameSSBOData_.objectSSBOData.push_back({backgroundTiling});
  }

  const auto& objects = grid_->getObjects();
  const auto& objectIds = grid_->getObjectIds();

  // Add padding objects
  int32_t paddingTileIdx = device_->getSpriteArrayLayer("_padding_");
  if (paddingTileIdx != -1) {
    for (int32_t xPad = observableGrid.right - gridWidth_; xPad < observableGrid.left + static_cast<int32_t>(gridWidth_); xPad++) {
      for (int32_t yPad = observableGrid.top - gridHeight_; yPad < observableGrid.bottom + static_cast<int32_t>(gridHeight_); yPad++) {
        spdlog::debug("xpad,ypad {0},{1}", xPad, yPad);
        if (xPad < 0 || yPad < 0 || xPad >= gridBoundary_.x || yPad >= gridBoundary_.y) {
          spdlog::debug("Adding padding tile at {0},{1}", xPad, yPad);
          vk::ObjectDataSSBO objectData{};
          objectData.textureIndex = paddingTileIdx;
          objectData.gridPosition = {xPad, yPad, -10, 0};
          // Translate the locations with respect to global transform
          glm::vec4 renderLocation = globalModelMatrix * glm::vec4(xPad, yPad, 0.0, 1.0);
          spdlog::debug("Rendering padding tile at {0},{1}", renderLocation.x, renderLocation.y);

          // Translate
          objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
          objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)
          frameSSBOData_.objectSSBOData.push_back({objectData});
        }
      }
    }
  }

  for (auto& object : objects) {
    vk::ObjectDataSSBO objectData{};
    std::vector<vk::ObjectVariableSSBO> objectVariableData{};

    const auto& location = object->getLocation();

    const auto& objectName = object->getObjectName();

    spdlog::debug("Updating object {0} at location [{1},{2}]", objectName, location.x, location.y);

    // Check we are within the boundary of the render grid
    if (location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top) {
      continue;
    }

    auto objectOrientation = object->getObjectOrientation();

    const auto& tileName = object->getObjectRenderTileName();
    auto objectPlayerId = object->getPlayerId();

    auto objectTypeId = objectIds.at(objectName);
    auto zIdx = object->getZIdx();

    if (spriteDefinitions_.find(tileName) == spriteDefinitions_.end()) {
      throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, objectName));
    }

    const auto& spriteDefinition = spriteDefinitions_.at(tileName);
    auto tilingMode = spriteDefinition.tilingMode;
    auto isWallTiles = tilingMode != TilingMode::NONE;

    // Translate the locations with respect to global transform
    glm::vec4 renderLocation = globalModelMatrix * glm::vec4(location, 0.0, 1.0);

    // Translate
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)

    // Rotate the objects that should be rotated
    if (config_.rotateAvatarImage) {
      if (!(object == avatarObject_ && config_.rotateWithAvatar) && !isWallTiles) {
        auto objectAngleRadians = objectOrientation.getAngleRadians() - globalOrientation.getAngleRadians();
        objectData.modelMatrix = glm::rotate(objectData.modelMatrix, objectAngleRadians, glm::vec3(0.0, 0.0, 1.0));
      }
    }

    // Scale the objects based on their scales
    auto scale = spriteDefinition.scale;
    objectData.modelMatrix = glm::scale(objectData.modelMatrix, glm::vec3(scale, scale, 1.0));

    auto spriteName = getSpriteName(objectName, tileName, location, globalOrientation.getDirection());
    objectData.textureIndex = device_->getSpriteArrayLayer(spriteName);
    objectData.playerId = objectPlayerId;
    objectData.gridPosition = {location.x, location.y, zIdx, 0};
    objectData.objectTypeId = objectTypeId;

    for (auto variableValue : getExposedVariableValues(object)) {
      objectVariableData.push_back({variableValue});
    }

    frameSSBOData_.objectSSBOData.push_back({objectData, objectVariableData});
  }

  // Sort by z-index, so we render things on top of each other in the right order
  std::sort(frameSSBOData_.objectSSBOData.begin(), frameSSBOData_.objectSSBOData.end(),
            [this](const vk::ObjectSSBOs& a, const vk::ObjectSSBOs& b) -> bool {
              return a.objectData.gridPosition.z < b.objectData.gridPosition.z;
            });
}

void SpriteObserver::updateCommandBuffer() {
  for (int i = 0; i < frameSSBOData_.objectSSBOData.size(); i++) {
    device_->updateObjectPushConstants(i);
  }
}

}  // namespace griddly
