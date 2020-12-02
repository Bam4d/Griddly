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

VulkanGridObserver::VulkanGridObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig) : VulkanObserver(grid, resourceConfig) {
}

VulkanGridObserver::~VulkanGridObserver() {
}

void VulkanGridObserver::resetRenderSurface() {
  // Delete old render surfaces (if they exist)

  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto tileSize = observerConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
  observationStrides_ = {1, 3, 3 * pixelWidth_};

  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}. Pixel width={2}. height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);

  device_->resetRenderSurface(pixelWidth_, pixelHeight_);
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
        std::max(0, (int32_t)location.x * tileSize.x - 2),
        std::max(0, (int32_t)location.y * tileSize.y - 2)};

    // Because we make the dirty rectangles slightly larger than the sprites, must check boundaries do not go beyond
    // the render image surface
   // Because we make the dirty rectangles slightly larger than the sprites, must check boundaries do not go beyond 
    // the render image surface
    auto extentWidth = (uint32_t)tileSize.x + 4;
    auto boundaryX = (int32_t)extentWidth + offset.x - (int32_t)pixelWidth_;
    if (boundaryX > 0) {
     extentWidth -= boundaryX;
    }

    auto extentHeight = (uint32_t)tileSize.y + 4;
    auto boundaryY = (int32_t)extentHeight + offset.y - (int32_t)pixelHeight_;
    if (boundaryY > 0) {
     extentHeight -= boundaryY;
    }

    VkExtent2D extent;
    extent.width = extentWidth;
    extent.height = extentHeight;

    dirtyRectangles.push_back({offset, extent});
  }

  return dirtyRectangles;
}

void VulkanGridObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = observerConfig_.tileSize;
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

      auto maxx = pGrid.right - pGrid.left;
      auto maxy = pGrid.top - pGrid.bottom;

      switch (avatarDirection) {
        default:
        case Direction::UP:
        case Direction::NONE: {
          auto objy = pGrid.bottom;
          for (auto outy = 0; outy <= maxy; outy++) {
            auto objx = pGrid.left;
            for (auto outx = 0; outx <= maxx; outx++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              objx++;
            }
            objy++;
          }
        } break;
        case Direction::DOWN: {
          auto objy = pGrid.top;
          for (auto outy = 0; outy <= maxy; outy++) {
            auto objx = pGrid.right;
            for (auto outx = 0; outx <= maxx; outx++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              objx--;
            }
            objy--;
          }
        } break;
        case Direction::RIGHT: {
          auto objx = pGrid.right;
          for (auto outy = 0; outy <= maxx; outy++) {
            auto objy = pGrid.bottom;
            for (auto outx = 0; outx <= maxy; outx++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              objy++;
            }
            objx--;
          }
        } break;
        case Direction::LEFT: {
          auto objx = pGrid.left;
          for (auto outy = 0; outy <= maxx; outy++) {
            auto objy = pGrid.top;
            for (auto outx = 0; outx <= maxy; outx++) {
              renderLocation(ctx, {objx, objy}, {outx, outy}, tileOffset, avatarDirection);
              objy--;
            }
            objx++;
          }
        } break;
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
  }  // namespace griddly
  else {
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
}  // namespace griddly
}  // namespace griddly