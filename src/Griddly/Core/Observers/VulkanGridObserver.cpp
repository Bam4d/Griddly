#include "VulkanGridObserver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>

#include "../Grid.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace griddly {

VulkanGridObserver::VulkanGridObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig) : VulkanObserver(grid, resourceConfig) {
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

void VulkanGridObserver::init(ObserverConfig observerConfig) {
  VulkanObserver::init(observerConfig);

  uint32_t players = 1;
  for (auto object : grid_->getObjects()) {
    if (object->getPlayerId() > players) {
      players = object->getPlayerId();
    }
  }

  float s = 1.0;
  float v = 0.6;
  float h_inc = 360.0 / players;
  for (int p = 0; p < players; p++) {
    int h = h_inc * p;
    glm::vec4 rgba = glm::vec4(glm::rgbColor(glm::vec3(h, s, v)), 1.0);
    globalObserverPlayerColors_.push_back(rgba);
  }
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

void VulkanGridObserver::render(vk::VulkanRenderContext& ctx) const {
  auto tileSize = observerConfig_.tileSize;
  auto tileOffset = static_cast<glm::vec2>(tileSize) / 2.0f;
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
  } else {
    auto& updatedLocations = grid_->getUpdatedLocations(observerConfig_.playerId);

    for (auto& location : updatedLocations) {
      if (location.x >= observerConfig_.gridXOffset &&
          location.x < gridWidth_ + observerConfig_.gridXOffset &&
          location.y >= observerConfig_.gridYOffset &&
          location.y < gridHeight_ + observerConfig_.gridYOffset) {
        auto outputLocation = glm::ivec2(
            location.x - observerConfig_.gridXOffset,
            location.y - observerConfig_.gridYOffset);

        if (outputLocation.x < gridWidth_ && outputLocation.x >= 0 && outputLocation.y < gridHeight_ && outputLocation.y >= 0) {
          renderLocation(ctx, location, outputLocation, tileOffset, Direction::NONE);
        }
      }
    }
  }
}

}  // namespace griddly