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

  glm::vec2 midpoint(static_cast<float>(gridWidth_)/2.0 -0.5, static_cast<float>(gridHeight_)/2.0 -0.5);


   isoTransform_ = isoTransform_ * glm::mat4({
      {0.5f, -0.5f * isoHeightRatio, 0, 0},
      {0.5f, 0.5f * isoHeightRatio, 0, 0},
      {0, 0, 1.0f, 0},
      {0, 1.0-isoHeightRatio, 0, 1.0f},
  });

  // Translate back to center
  isoTransform_ = glm::translate(isoTransform_, glm::vec3(midpoint, 0.0));

  // Rotate 45 degrees for isometric tiles
  isoTransform_ = glm::rotate(isoTransform_, glm::pi<float>()/2.0f, glm::vec3(0.0, 0.0, 1.0));

  // Translate to origin
  isoTransform_ = glm::translate(isoTransform_, glm::vec3(-midpoint, 0.0));

  // auto tileSize = observerConfig_.tileSize;

  // auto tilePosition = glm::vec2(
  //     tileSize.x / 2.0f,
  //     observerConfig_.isoTileHeight / 2.0f);

  // const glm::mat2 isoMat = {
  //     {1.0, -1.0},
  //     {1.0, 1.0},
  // };

  // return localOffset + isoOriginOffset_ + outputLocation * isoMat * tilePosition;
}

void IsometricSpriteObserver::addBackgroundTile(std::vector<vk::ObjectDataSSBO>& objectDataSSBOData) {
  // vk::ObjectDataSSBO backgroundTiling;
  // backgroundTiling.modelMatrix = glm::translate(backgroundTiling.modelMatrix, glm::vec3(gridWidth_ / 2.0, gridHeight_ / 2.0, 0.0));
  // backgroundTiling.modelMatrix = glm::scale(backgroundTiling.modelMatrix, glm::vec3(gridWidth_, gridHeight_, 1.0));
  // backgroundTiling.zIdx = -1;
  // backgroundTiling.textureMultiply = {gridWidth_, gridHeight_};
  // backgroundTiling.textureIndex = device_->getSpriteArrayLayer("_iso_background_");

  // objectDataSSBOData.push_back(backgroundTiling);
}

std::vector<vk::ObjectDataSSBO> IsometricSpriteObserver::updateObjectSSBOData(PartialObservableGrid& observableGrid, glm::mat4& globalModelMatrix, DiscreteOrientation globalOrientation) {
  auto isoGlobalModelMatrix =   isoTransform_ * globalModelMatrix;

  return SpriteObserver::updateObjectSSBOData(observableGrid, isoGlobalModelMatrix, globalOrientation);
}

}  // namespace griddly
