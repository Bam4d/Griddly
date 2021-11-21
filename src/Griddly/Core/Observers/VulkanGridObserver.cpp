#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

VulkanGridObserver::VulkanGridObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, ShaderVariableConfig shaderVariableConfig) : VulkanObserver(grid, resourceConfig, shaderVariableConfig) {
}

VulkanGridObserver::~VulkanGridObserver() {
}

void VulkanGridObserver::resetShape() {
  spdlog::debug("Resetting grid observer shape.");

  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = observerConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
}

glm::mat4 VulkanGridObserver::getViewMatrix() {
  glm::mat4 viewMatrix(1);

  viewMatrix = glm::scale(viewMatrix, glm::vec3(observerConfig_.tileSize, 1.0));
  viewMatrix = glm::translate(viewMatrix, glm::vec3(observerConfig_.gridXOffset, observerConfig_.gridYOffset, 0.0));

  return viewMatrix;
}

glm::mat4 VulkanGridObserver::getGlobalModelMatrix() {
  glm::mat4 globalModelMatrix(1);

  if (avatarObject_ != nullptr) {
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(gridWidth_ / 2.0 - 0.5, gridHeight_ / 2.0 - 0.5, 0.0));
    auto avatarLocation = avatarObject_->getLocation();

    if (observerConfig_.rotateWithAvatar) {
      globalModelMatrix = glm::rotate(globalModelMatrix, -avatarObject_->getObjectOrientation().getAngleRadians(), glm::vec3(0.0, 0.0, 1.0));
    }
    // Put the avatar in the center
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(-avatarLocation, 0.0));
  }

  return globalModelMatrix;
}

PartialObservableGrid VulkanGridObserver::getObservableGrid() {
  PartialObservableGrid observableGrid;
  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    if (observerConfig_.rotateWithAvatar) {
      observableGrid = getAvatarObservableGrid(avatarLocation, avatarObject_->getObjectOrientation().getDirection());
    } else {
      observableGrid = getAvatarObservableGrid(avatarLocation);
    }
  } else {
    observableGrid = {gridHeight_ - observerConfig_.gridYOffset-1, -observerConfig_.gridYOffset, -observerConfig_.gridXOffset, gridWidth_ + observerConfig_.gridXOffset - 1};
  }

  observableGrid.left = std::max(0, observableGrid.left);
  observableGrid.right = std::max(0, observableGrid.right);
  observableGrid.bottom = std::max(0, observableGrid.bottom);
  observableGrid.top = std::max(0, observableGrid.top);

  return observableGrid;
}

vk::FrameSSBOData VulkanGridObserver::updateFrameShaderBuffers() {
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

  auto globalOrientation = DiscreteOrientation();
  if (avatarObject_ != nullptr && observerConfig_.rotateWithAvatar) {
    globalOrientation = avatarObject_->getObjectOrientation();
  }

  PartialObservableGrid observableGrid = getObservableGrid();
  glm::mat4 globalModelMatrix = getGlobalModelMatrix();

  auto objectData = updateObjectSSBOData(observableGrid, globalModelMatrix, globalOrientation);

  if (commandBufferObjectsCount_ != objectData.size()) {
    commandBufferObjectsCount_ = objectData.size();
    shouldUpdateCommandBuffer_ = true;
  }

  frameSSBOData.objectDataSSBOData.insert(frameSSBOData.objectDataSSBOData.end(), objectData.begin(), objectData.end());

  return frameSSBOData;
}

}  // namespace griddly