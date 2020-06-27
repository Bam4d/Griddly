#include "VulkanGridObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

const std::vector<glm::vec4> VulkanGridObserver::globalObserverPlayerColors_ = {
    {1.0, 0.0, 0.0, 0.5},
    {0.0, 0.0, 1.0, 0.5},
    {1.0, 0.0, 1.0, 0.5},
    {1.0, 1.0, 0.0, 0.5},
    {0.0, 1.0, 1.0, 0.5},
    {1.0, 1.0, 1.0, 0.5},
    {1.0, 0.0, 5.0, 0.5},
    {0.5, 0.0, 1.0, 0.5},
    {0.5, 1.0, 0.0, 0.5},
    {1.0, 0.5, 0.0, 0.5},
};

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
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarObject_->getObjectOrientation().getDirection();

    if (observerConfig_.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getAvatarObservableGrid(avatarLocation, avatarDirection);

      int32_t outx = 0, outy = 0;
      switch (avatarDirection) {
        default:
        case Direction::UP:
        case Direction::NONE:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              outy++;
            }
            outx++;
          }
          break;
        case Direction::DOWN:
          outx = observerConfig_.gridWidth - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = observerConfig_.gridHeight - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outy = observerConfig_.gridHeight - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              outx++;
            }
            outy--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = observerConfig_.gridWidth - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              outx--;
            }
            outy++;
          }
          break;
      }

    } else {
      auto pGrid = getAvatarObservableGrid(avatarLocation, Direction::NONE);
      int32_t outx = 0, outy = 0;
      for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
        outy = 0;
        for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
          renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, Direction::NONE);
          outy++;
        }
        outx++;
      }
    }
  } else {
    // TODO: Because this observation is not actually moving we can almost certainly optimize this to only update the updated locations
    if (observerConfig_.gridXOffset != 0 || observerConfig_.gridYOffset != 0) {
      auto left = observerConfig_.gridXOffset;
      auto right = observerConfig_.gridXOffset + observerConfig_.gridWidth;
      auto bottom = observerConfig_.gridYOffset;
      auto top = observerConfig_.gridYOffset + observerConfig_.gridHeight;
      int32_t outx = 0, outy = 0;
      for (auto objx = left; objx <= right; objx++) {
        outy = 0;
        for (auto objy = bottom; objy <= top; objy++) {
          renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, Direction::NONE);
          outy++;
        }
        outx++;
      }
    } else {
      auto updatedLocations = grid_->getUpdatedLocations();

      for (auto location : updatedLocations) {
        renderLocation(ctx, location, location, tileOffset, Direction::NONE);
      }
    }
  }
}
}  // namespace griddly