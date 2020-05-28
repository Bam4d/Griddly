#include "VulkanGridObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

VulkanGridObserver::VulkanGridObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig) : VulkanObserver(grid, vulkanObserverConfig) {
}

VulkanGridObserver::~VulkanGridObserver() {
}

std::shared_ptr<uint8_t> VulkanGridObserver::reset() const {
  auto tileSize = vulkanObserverConfig_.tileSize;
  auto ctx = device_->beginRender();

  render(ctx);

  auto width = observerConfig_.gridWidth * tileSize;
  auto height = observerConfig_.gridHeight * tileSize;

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles = {
      {{0, 0},
       {width, height}}};

  return device_->endRender(ctx, dirtyRectangles);
}

std::shared_ptr<uint8_t> VulkanGridObserver::update(int playerId) const {
  auto ctx = device_->beginRender();
  auto tileSize = vulkanObserverConfig_.tileSize;

  render(ctx);

  std::vector<VkRect2D> dirtyRectangles;

  // Optimize this in the future, partial observation is slower for the moment
  if (avatarObject_ != nullptr) {
    auto width = observerConfig_.gridWidth * tileSize;
    auto height = observerConfig_.gridHeight * tileSize;

    std::vector<VkRect2D> dirtyRectangles = {
        {{0, 0},
         {width, height}}};

    return device_->endRender(ctx, dirtyRectangles);
  }

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto location : updatedLocations) {
    // If the observation window is smaller than the actual grid for some reason, dont try to render the off-image things
    if (observerConfig_.gridHeight <= location.y || observerConfig_.gridWidth <= location.x) {
      continue;
    }

    VkOffset2D offset = {(int32_t)(location.x * tileSize), (int32_t)(location.y * tileSize)};
    VkExtent2D extent = {tileSize, tileSize};
    dirtyRectangles.push_back({offset, extent});
  }

  return device_->endRender(ctx, dirtyRectangles);
}

void VulkanGridObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = (float)vulkanObserverConfig_.tileSize;
  auto tileOffset = tileSize / 2.0f;
  // Just change the viewport of the renderer to point at the correct place
  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();

    // Assuming here that gridWidth and gridHeight are odd numbers
    auto gridLeft = (int32_t)avatarLocation.x - (int32_t)(observerConfig_.gridWidth - 1) / 2;
    auto gridRight = (int32_t)avatarLocation.x + (int32_t)(observerConfig_.gridWidth - 1) / 2;

    auto gridBottom = (int32_t)avatarLocation.y - (int32_t)(observerConfig_.gridHeight - 1) / 2;
    auto gridTop = (int32_t)avatarLocation.y + (int32_t)(observerConfig_.gridHeight - 1) / 2;

    auto leftPixels = (float)gridLeft * tileSize;
    auto rightPixels = (float)(gridRight + 1) * tileSize;
    auto topPixels = (float)(gridTop + 1) * tileSize;
    auto bottomPixels = (float)gridBottom * tileSize;

    glm::mat4 viewMatrix = glm::ortho(leftPixels, rightPixels, bottomPixels, topPixels, 0.0f, 1.0f);
    ctx.viewMatrix = viewMatrix;

    uint32_t renderGridLeft = gridLeft > 0 ? gridLeft : 0;
    uint32_t renderGridRight = gridRight > grid_->getWidth() ? gridRight : grid_->getWidth();
    uint32_t renderGridBottom = gridBottom > 0 ? gridBottom : 0;
    uint32_t renderGridTop = gridTop > grid_->getHeight() ? gridTop : grid_->getHeight();

    for (auto x = renderGridLeft; x <= renderGridRight; x++) {
      for (auto y = renderGridBottom; y <= renderGridTop; y++) {
        renderLocation(ctx, {x, y}, tileSize, tileOffset);
      }
    }
    return;
  }

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto location : updatedLocations) {
    renderLocation(ctx, location, tileSize, tileOffset);
  }
}
}