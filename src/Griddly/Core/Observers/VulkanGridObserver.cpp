#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "VulkanGridObserver.hpp"

namespace griddly {

VulkanGridObserver::VulkanGridObserver(std::shared_ptr<Grid> grid) : VulkanObserver(std::move(grid)) {
}

void VulkanGridObserver::init(GridObserverConfig& config) {
  VulkanObserver::init(config);
  gridObserverConfig_ = config;
}

void VulkanGridObserver::resetShape() {
  spdlog::debug("Resetting grid observer shape.");

  gridWidth_ = gridObserverConfig_.overrideGridWidth > 0 ? gridObserverConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = gridObserverConfig_.overrideGridHeight > 0 ? gridObserverConfig_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = gridObserverConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
}

glm::mat4 VulkanGridObserver::getViewMatrix() {
  glm::mat4 viewMatrix(1);
  viewMatrix = glm::scale(viewMatrix, glm::vec3(gridObserverConfig_.tileSize, 1.0));
  viewMatrix = glm::translate(viewMatrix, glm::vec3(gridObserverConfig_.gridXOffset, gridObserverConfig_.gridYOffset, 0.0));
  return viewMatrix;
}

glm::mat4 VulkanGridObserver::getGlobalModelMatrix() {
  glm::mat4 globalModelMatrix(1);

  if (avatarObject_ != nullptr) {
    globalModelMatrix = glm::translate(globalModelMatrix, glm::vec3(gridWidth_ / 2.0 - 0.5, gridHeight_ / 2.0 - 0.5, 0.0));
    auto avatarLocation = avatarObject_->getLocation();

    if (gridObserverConfig_.rotateWithAvatar) {
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
    if (gridObserverConfig_.rotateWithAvatar) {
      observableGrid = getAvatarObservableGrid(avatarLocation, avatarObject_->getObjectOrientation().getDirection());
    } else {
      observableGrid = getAvatarObservableGrid(avatarLocation);
    }
  } else {
    observableGrid = {
        static_cast<int32_t>(gridHeight_) - static_cast<int32_t>(gridObserverConfig_.gridYOffset) - 1,
        -gridObserverConfig_.gridYOffset,
        -gridObserverConfig_.gridXOffset,
        static_cast<int32_t>(gridWidth_) + static_cast<int32_t>(gridObserverConfig_.gridXOffset) - 1};
  }

  observableGrid.left = std::max(0, observableGrid.left);
  observableGrid.right = std::max(0, observableGrid.right);
  observableGrid.bottom = std::max(0, observableGrid.bottom);
  observableGrid.top = std::max(0, observableGrid.top);

  return observableGrid;
}

std::vector<int32_t> VulkanGridObserver::getExposedVariableValues(std::shared_ptr<Object> object) {
  std::vector<int32_t> variableValues;
  for (auto variableName : gridObserverConfig_.shaderVariableConfig.exposedObjectVariables) {
    auto variableValuePtr = object->getVariableValue(variableName);
    if (variableValuePtr != nullptr) {
      variableValues.push_back(*variableValuePtr);
    } else {
      variableValues.push_back(0);
    }
  }

  return variableValues;
}

void VulkanGridObserver::updateFrameShaderBuffers() {
  auto globalVariables = grid_->getGlobalVariables();

  // TODO: do we always need to clear these? Probably more efficient to clear them.
  frameSSBOData_.globalVariableSSBOData.clear();
  for (auto globalVariableName : gridObserverConfig_.shaderVariableConfig.exposedGlobalVariables) {
    auto globalVariablesPerPlayerIt = globalVariables.find(globalVariableName);

    if (globalVariablesPerPlayerIt == globalVariables.end()) {
      auto error = fmt::format("Global variable '{0}' cannot be passed to shader as it cannot be found.", globalVariableName);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    auto globalVariablesPerPlayer = globalVariablesPerPlayerIt->second;
    auto playerVariablesIt = globalVariablesPerPlayer.find(gridObserverConfig_.playerId);

    int32_t value;
    if (playerVariablesIt == globalVariablesPerPlayer.end()) {
      value = *globalVariablesPerPlayer.at(0);
    } else {
      value = *playerVariablesIt->second;
    }

    spdlog::debug("Adding global variable {0}, value: {1} ", globalVariableName, value);
    frameSSBOData_.globalVariableSSBOData.push_back(vk::GlobalVariableSSBO{value});
  }

  auto globalOrientation = DiscreteOrientation();
  if (avatarObject_ != nullptr && gridObserverConfig_.rotateWithAvatar) {
    globalOrientation = avatarObject_->getObjectOrientation();
  }

  PartialObservableGrid observableGrid = getObservableGrid();
  glm::mat4 globalModelMatrix = getGlobalModelMatrix();

  frameSSBOData_.objectSSBOData.clear();
  updateObjectSSBOData(observableGrid, globalModelMatrix, globalOrientation);

  if (commandBufferObjectsCount_ != frameSSBOData_.objectSSBOData.size()) {
    commandBufferObjectsCount_ = frameSSBOData_.objectSSBOData.size();
    shouldUpdateCommandBuffer_ = true;
  }
}

}  // namespace griddly