#include "VulkanGridObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

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

  PartialObservableGrid observableGrid;
  auto globalOrientation = DiscreteOrientation();

  // If we conter on the avatar, use offsets, or change the orientation of the avatar
  glm::mat4 globalModelMatrix{1};

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    observableGrid = getAvatarObservableGrid(avatarLocation, globalOrientation.getDirection());

    // Put the avatar in the center
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(gridWidth_ / 2.0 - 0.5, gridHeight_ / 2.0 - 0.5, 0.0));

    if (observerConfig_.rotateWithAvatar) {
      globalOrientation = avatarObject_->getObjectOrientation();
      observableGrid = getAvatarObservableGrid(avatarLocation, globalOrientation.getDirection());
      globalModelMatrix = glm::rotate(globalModelMatrix, -globalOrientation.getAngleRadians(), glm::vec3(0.0, 0.0, 1.0));
    }

    // Move avatar to 0,0
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(-avatarLocation, 0.0));

  } else {
    observableGrid = {gridHeight_ - observerConfig_.gridYOffset, observerConfig_.gridYOffset, observerConfig_.gridXOffset, gridWidth_ - observerConfig_.gridXOffset};
  }

  auto objectData = updateObjectSSBOData(observableGrid, globalModelMatrix, globalOrientation);

  if (commandBufferObjectsCount_ != objectData.size()) {
    commandBufferObjectsCount_ = objectData.size();
    shouldUpdateCommandBuffer_ = true;
  }

  frameSSBOData.objectDataSSBOData.insert(frameSSBOData.objectDataSSBOData.end(), objectData.begin(), objectData.end());

  return frameSSBOData;
}

}  // namespace griddly