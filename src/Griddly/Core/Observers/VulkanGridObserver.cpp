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
    // Assuming here that gridWidth and gridHeight are odd numbers
    int32_t gridLeft, gridRight, gridBottom, gridTop;
    switch (avatarOrientation) {
      case Direction::NONE:
      case Direction::UP:
        gridLeft = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridRight = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridBottom = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
        gridTop = (int32_t)avatarLocation.y - (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
        break;
      case Direction::RIGHT:
        gridLeft = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridRight = (int32_t)avatarLocation.x - (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridBottom = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
        gridTop = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
        break;
      case Direction::DOWN:
        gridLeft = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridRight = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridBottom = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
        gridTop = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
        break;
      case Direction::LEFT:
        gridLeft = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridYOffset - (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridRight = (int32_t)avatarLocation.x + (int32_t)observerConfig_.gridYOffset + (int32_t)(observerConfig_.gridWidth - 1) / 2;
        gridBottom = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset - (int32_t)(observerConfig_.gridHeight - 1) / 2;
        gridTop = (int32_t)avatarLocation.y + (int32_t)observerConfig_.gridXOffset + (int32_t)(observerConfig_.gridHeight - 1) / 2;
        break;
    }

    auto leftPixels = (float)gridLeft * tileSize;
    auto rightPixels = (float)(gridRight + 1) * tileSize;
    auto topPixels = (float)(gridTop + 1) * tileSize;
    auto bottomPixels = (float)gridBottom * tileSize;

    glm::mat4 viewMatrix = glm::ortho(0.0f, (float)observerConfig_.gridWidth, 0.0f, (float)observerConfig_.gridHeight, 0.0f, 1.0f);
    ctx.viewMatrix = viewMatrix;

    uint32_t renderGridLeft = gridLeft > 0 ? gridLeft : 0;
    uint32_t renderGridRight = gridRight > grid_->getWidth() ? gridRight : grid_->getWidth();
    uint32_t renderGridBottom = gridBottom > 0 ? gridBottom : 0;
    uint32_t renderGridTop = gridTop > grid_->getHeight() ? gridTop : grid_->getHeight();

    uint32_t outx = 0, outy = 0;
    if (!observerConfig_.rotateWithAvatar) {
      for (auto objx = renderGridLeft; objx <= renderGridRight; objx++) {
        for (auto objy = renderGridBottom; objy <= renderGridTop; objy++) {
          renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, avatarOrientation);
          outy++;
        }
        outx++;
      }
    }

    switch (avatarOrientation) {
      case Direction::UP:
      case Direction::NONE:
        for (auto objx = renderGridLeft; objx <= renderGridRight; objx++) {
          for (auto objy = renderGridBottom; objy <= renderGridTop; objy++) {
            renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, Direction::NONE);
            outy++;
          }
          outx++;
        }
        break;
      case Direction::DOWN:
        outy = observerConfig_.gridHeight;
        for (auto objx = renderGridLeft; objx <= renderGridRight; objx++) {
          for (auto objy = renderGridBottom; objy <= renderGridTop; objy++) {
            renderLocation(ctx, {objx, objy}, {outx, outy}, tileSize, tileOffset, Direction::NONE);
            outy--;
          }
          outx++;
        }
        break;
    }

    return;
  }

  auto updatedLocations = grid_->getUpdatedLocations();

  for (auto location : updatedLocations) {
    renderLocation(ctx, location, location, tileSize, tileOffset, Direction::NONE);
  }
}
}  // namespace griddly