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

float VulkanGridObserver::getObjectRotation(std::shared_ptr<Object> object) const {
  auto objectOrientation = object->getObjectOrientation();

  // If we are rotating with the avatar then we dont rotate the avatar object
  if (object == avatarObject_ && observerConfig_.rotateWithAvatar) {
    return 0.0f;
  }

  float objectRotationRad;
  switch (objectOrientation) {
    case Direction::NONE:
    case Direction::DOWN:
      return 0;
      break;
    case Direction::RIGHT:
      return glm::pi<float>() / 2.0f;
      break;
    case Direction::UP:
      return glm::pi<float>();
      break;
    case Direction::LEFT:
      return 3.0 * glm::pi<float>() / 2.0f;
      break;
  }
}

void VulkanGridObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = (float)vulkanObserverConfig_.tileSize;
  auto tileOffset = tileSize / 2.0f;
  // Just change the viewport of the renderer to point at the correct place
  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();

    if (observerConfig_.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getPartialObservableGrid(avatarLocation, avatarOrientation);

      uint32_t outx = 0, outy = 0;
      switch (avatarOrientation) {
        default:
        case Direction::DOWN:
        case Direction::NONE:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, avatarOrientation);
              outy++;
            }
            outx++;
          }
          break;
        case Direction::UP:
          outx = observerConfig_.gridWidth - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = observerConfig_.gridHeight - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, avatarOrientation);
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outx = observerConfig_.gridWidth - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outy, outx}, tileSize, tileOffset, avatarOrientation);
              outy++;
            }
            outx--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = observerConfig_.gridHeight - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outy, outx}, tileSize, tileOffset, avatarOrientation);
              outy--;
            }
            outx++;
          }
          break;
      }

    } else {

      auto pGrid = getPartialObservableGrid(avatarLocation, Direction::NONE);
      uint32_t outx = 0, outy = 0;
      for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
        outy = 0;
        for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
          renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, Direction::NONE);
          outy++;
        }
        outx++;
      }
      return;
    }

    return;
  }

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto location : updatedLocations) {
    renderLocation(ctx, location, location, tileSize, tileOffset, Direction::NONE);
  }
}
}  // namespace griddly