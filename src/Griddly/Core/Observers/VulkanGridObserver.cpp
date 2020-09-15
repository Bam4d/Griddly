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

void VulkanGridObserver::resetRenderSurface() {
  // Delete old render surfaces (if they exist)

  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto tileSize = vulkanObserverConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
  observationStrides_ = {1, 3, 3 * pixelWidth_};

  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}", gridWidth_, gridHeight_);

  device_->resetRenderSurface(pixelWidth_, pixelHeight_);
}

std::vector<VkRect2D> VulkanGridObserver::calculateDirtyRectangles(std::unordered_set<glm::ivec2> updatedLocations) const {
  auto tileSize = vulkanObserverConfig_.tileSize;
  std::vector<VkRect2D> dirtyRectangles;

  for (auto location : updatedLocations) {
    // If the observation window is smaller than the actual grid for some reason, dont try to render the off-image things
    if (gridHeight_ <= location.y || gridWidth_ <= location.x) {
      continue;
    }

    VkOffset2D offset = {(int32_t)(location.x * tileSize.x - 2), (int32_t)(location.y * tileSize.y - 2)};
    VkExtent2D extent = {tileSize.x + 2, tileSize.y + 2};
    dirtyRectangles.push_back({offset, extent});
  }
}

void VulkanGridObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = vulkanObserverConfig_.tileSize;
  auto tileOffset = (glm::vec2)tileSize / 2.0f;
  // Just change the viewport of the renderer to point at the correct place
  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarOrientation.getDirection();

    spdlog::debug("Avatar orientation for rendering [{0}, {1}] {2}", avatarOrientation.getUnitVector().x, avatarOrientation.getUnitVector().y, avatarDirection);

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
          outx = gridWidth_ - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = gridHeight_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outy = gridHeight_ - 1;
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
            outx = gridWidth_ - 1;
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
    //if (observerConfig_.gridXOffset != 0 || observerConfig_.gridYOffset != 0) {
    auto left = observerConfig_.gridXOffset;
    auto right = observerConfig_.gridXOffset + gridWidth_;
    auto bottom = observerConfig_.gridYOffset;
    auto top = observerConfig_.gridYOffset + gridHeight_;
    int32_t outx = 0, outy = 0;
    for (auto objx = left; objx < right; objx++) {
      outy = 0;
      for (auto objy = bottom; objy < top; objy++) {
        renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, Direction::NONE);
        outy++;
      }
      outx++;
    }
    // } else {
    //   auto updatedLocations = grid_->getUpdatedLocations();

    //   for (auto location : updatedLocations) {
    //     renderLocation(ctx, location, location, tileOffset, Direction::NONE);
    //   }
    // }
  }
}
}  // namespace griddly