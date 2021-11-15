#include "SpriteObserver.hpp"

// Have to define this so the image loader is compiled
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

SpriteObserver::SpriteObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, std::unordered_map<std::string, SpriteDefinition> spriteDefinitions, ShaderVariableConfig shaderVariableConfig) : VulkanGridObserver(grid, resourceConfig, shaderVariableConfig), spriteDefinitions_(spriteDefinitions) {
}

SpriteObserver::~SpriteObserver() {
}

ObserverType SpriteObserver::getObserverType() const {
  return ObserverType::SPRITE_2D;
}

// Load a single texture
vk::SpriteData SpriteObserver::loadImage(std::string imageFilename) {
  int width, height, channels;

  std::string absoluteFilePath = resourceConfig_.imagePath + "/" + imageFilename;
  spdlog::debug("Loading Sprite {0}", absoluteFilePath);
  stbi_uc* pixels = stbi_load(absoluteFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error(fmt::format("Failed to load texture image {0}.", imageFilename));
  }

  int outputWidth = observerConfig_.tileSize.x;
  int outputHeight = observerConfig_.tileSize.y;

  stbi_uc* resizedPixels = (stbi_uc*)malloc(outputWidth * outputHeight * 4);

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

  device_->initRenderMode(vk::RenderMode::SPRITES);

  std::unordered_map<std::string, vk::SpriteData> spriteData;
  for (auto spriteDefinitionIt : spriteDefinitions_) {
    auto spriteDefinition = spriteDefinitionIt.second;
    auto spriteName = spriteDefinitionIt.first;
    auto spriteImages = spriteDefinition.images;

    if (spriteDefinition.tilingMode == TilingMode::WALL_2 || spriteDefinition.tilingMode == TilingMode::WALL_16) {
      if (spriteDefinition.tilingMode == TilingMode::WALL_2 && spriteImages.size() != 2 || spriteDefinition.tilingMode == TilingMode::WALL_16 && spriteImages.size() != 16) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectively. {0} images were supplied", spriteImages.size()));
      }

      for (int s = 0; s < spriteImages.size(); s++) {
        auto spriteNameAndIdx = spriteName + std::to_string(s);
        spriteData.insert({spriteNameAndIdx, loadImage(spriteDefinition.images[s])});
      }
    } else {
      spriteData.insert({spriteName, loadImage(spriteDefinition.images[0])});
    }
  }

  device_->preloadSprites(spriteData);
}

std::string SpriteObserver::getSpriteName(std::string objectName, std::string tileName, glm::ivec2 location, Direction orientation) const {
  auto tilingMode = spriteDefinitions_.at(tileName).tilingMode;

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

vk::FrameSSBOData SpriteObserver::updateFrameShaderBuffers() {

  vk::FrameSSBOData frameSSBOData;

  auto globalVariables = grid_->getGlobalVariables();
  for (auto globalVariableName : shaderVariableConfig_.exposedGlobalVariables) {
    auto globalVariablesPerPlayer = globalVariables.at(globalVariableName);
    auto playerVariablesIt = globalVariablesPerPlayer.find(observerConfig_.playerId);

    int32_t value;
    if (playerVariablesIt == globalVariablesPerPlayer.end()) {
      value = *globalVariablesPerPlayer.at(0);
    } else {
      value = *playerVariablesIt->second;
    }

    spdlog::debug("Adding global variable {0}, value: {1} ", globalVariableName, value);
    frameSSBOData.globalVariableSSBOData.push_back(vk::GlobalVariableSSBO{value});
  }

  PartialObservableGrid observableGrid;
  auto gridOrientation = DiscreteOrientation();

  // If we conter on the avatar, use offsets, or change the orientation of the avatar
  glm::mat4 globalModelMatrix{1};

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    observableGrid = getAvatarObservableGrid(avatarLocation, gridOrientation.getDirection());

    // Put the avatar in the center
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(gridWidth_ / 2.0 - 0.5, gridHeight_ / 2.0 - 0.5, 0.0));

    if (observerConfig_.rotateWithAvatar) {
      gridOrientation = avatarObject_->getObjectOrientation();
      observableGrid = getAvatarObservableGrid(avatarLocation, gridOrientation.getDirection());
      globalModelMatrix = glm::rotate(globalModelMatrix, -gridOrientation.getAngleRadians(), glm::vec3(0.0, 0.0, 1.0));
    }

    // Move avatar to 0,0
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(-avatarLocation, 0.0));

  } else {
    observableGrid = {gridHeight_ - observerConfig_.gridYOffset, observerConfig_.gridYOffset, observerConfig_.gridXOffset, gridWidth_ - observerConfig_.gridXOffset};
  }

  // Background object to be object 0
  spdlog::debug("Grid: {0}, {1} ", gridWidth_, gridHeight_);
  vk::ObjectDataSSBO backgroundTiling;
  backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(gridWidth_ / 2.0, gridHeight_ / 2.0, 0.0));
  backgroundTiling.modelMatrix = glm::scale(backgroundTiling.modelMatrix, glm::vec3(gridWidth_, gridHeight_, 1.0));
  backgroundTiling.zIdx = -1;
  backgroundTiling.textureMultiply = {gridWidth_, gridHeight_};
  backgroundTiling.textureIndex = device_->getSpriteArrayLayer("_background_");
  frameSSBOData.objectDataSSBOData.push_back(backgroundTiling);

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

    auto spriteDefinition = spriteDefinitions_.at(tileName);
    auto tilingMode = spriteDefinition.tilingMode;
    auto isWallTiles = tilingMode != TilingMode::NONE;

    // Translate the locations with respect to global transform
    glm::vec4 renderLocation = globalModelMatrix * glm::vec4(location, 0.0, 1.0);

    // Translate
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(renderLocation.x, renderLocation.y, 0.0));
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));  // Observer offsets
    objectData.modelMatrix = glm::translate(objectData.modelMatrix, glm::vec3(0.5, 0.5, 0.0));                                                  // Offset for the the vertexes as they are between (-0.5, 0.5) and we want them between (0, 1)

    // Rotate the objects that should be rotated
    if (!(object == avatarObject_ && observerConfig_.rotateWithAvatar) && !isWallTiles) {
      auto objectAngleRadians = objectOrientation.getAngleRadians() - gridOrientation.getAngleRadians();
      objectData.modelMatrix = glm::rotate(objectData.modelMatrix, objectAngleRadians, glm::vec3(0.0, 0.0, 1.0));
    }

    auto spriteName = getSpriteName(objectName, tileName, location, gridOrientation.getDirection());
    objectData.textureIndex = device_->getSpriteArrayLayer(spriteName);
    objectData.playerId = objectPlayerId;
    objectData.zIdx = zIdx;

    frameSSBOData.objectDataSSBOData.push_back(objectData);
  }

  // Sort by z-index, so we render things on top of each other in the right order
  std::sort(frameSSBOData.objectDataSSBOData.begin(), frameSSBOData.objectDataSSBOData.end(),
            [this](const vk::ObjectDataSSBO& a, const vk::ObjectDataSSBO& b) -> bool {
              return a.zIdx < b.zIdx;
            });

  if (commandBufferObjectsCount_ != ssboData.objectDataSSBOData.size()) {
    commandBufferObjectsCount_ = ssboData.objectDataSSBOData.size();
    shouldUpdateCommandBuffer_ = true;
  }

  return frameSSBOData;
}

}  // namespace griddly
