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

std::vector<VkRect2D> VulkanGridObserver::calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const {
  auto tileSize = observerConfig_.tileSize;
  std::vector<VkRect2D> dirtyRectangles;

  for (auto location : updatedLocations) {
    // If the observation window is smaller than the actual grid for some reason, dont try to render the off-image things
    if (gridHeight_ <= location.y || gridWidth_ <= location.x) {
      continue;
    }

    VkOffset2D offset = {
        std::max(0, location.x * tileSize.x),
        std::max(0, location.y * tileSize.y)};

    VkExtent2D extent = {
        static_cast<uint32_t>(tileSize.x),
        static_cast<uint32_t>(tileSize.y)};

    dirtyRectangles.push_back({offset, extent});
  }

  return dirtyRectangles;
}

}  // namespace griddly