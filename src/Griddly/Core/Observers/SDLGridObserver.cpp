#include "SDLGridObserver.hpp"

#include <spdlog/spdlog.h>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>

#include "../Grid.hpp"

namespace griddly {

SDLGridObserver::SDLGridObserver(std::shared_ptr<Grid> grid, SDLGridObserverConfig& config) : SDLObserver(std::move(grid), config) {
  config_ = config;
}

void SDLGridObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  SDLObserver::init(playerObservers);
}

void SDLGridObserver::resetShape() {
  spdlog::debug("Resetting grid observer shape.");

  gridWidth_ = config_.overrideGridWidth > 0 ? config_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config_.overrideGridHeight > 0 ? config_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto tileSize = config_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
}

glm::vec2 SDLGridObserver::getTilePixelCenter(glm::ivec2 location) const {
  // Matches the old view*globalModel matrices: tile (x,y) center in pixels,
  // with optional avatar tracking (translate to center, rotate world, translate back)
  glm::vec2 tilePos(location.x, location.y);

  if (doTrackAvatar_ && avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    glm::vec2 v = tilePos - glm::vec2(avatarLocation);
    if (config_.rotateWithAvatar) {
      // Exact quarter-turn rotation (orientations are discrete): float cos/sin of 90/270 degrees leave
      // ~1e-8 residues that shift tiles by a pixel and open seams between them.
      switch (avatarObject_->getObjectOrientation().getDirection()) {
        case Direction::RIGHT:
          v = glm::vec2(v.y, -v.x);
          break;
        case Direction::DOWN:
          v = glm::vec2(-v.x, -v.y);
          break;
        case Direction::LEFT:
          v = glm::vec2(-v.y, v.x);
          break;
        default:
          break;
      }
    }
    // OffsetX/Y are in cells in the (possibly rotated) view frame: getAvatarObservableGrid shifts the
    // visible window by -offset, so the avatar sits offset cells from the centre (e.g. OffsetY > 0 puts
    // the avatar lower in the view so it sees further ahead).
    tilePos = v + glm::vec2(gridWidth_ / 2.0f - 0.5f + config_.gridXOffset, gridHeight_ / 2.0f - 0.5f + config_.gridYOffset);
    return (tilePos + 0.5f) * glm::vec2(config_.tileSize);
  }

  return (tilePos + 0.5f) * glm::vec2(config_.tileSize) + glm::vec2(config_.gridXOffset, config_.gridYOffset);
}

double SDLGridObserver::getObjectAngle(std::shared_ptr<ObjectRules> object, DiscreteOrientation globalOrientation) const {
  if (!config_.rotateAvatarImage) {
    return 0.0;
  }
  // A sprite is drawn at the same angle in every view: its orientation in global (map) space. In an
  // egocentric view (RotateWithAvatar) only the grid positions rotate around the avatar, so objects look
  // exactly as on the map. (Previously sprites were counter-rotated by the view orientation, so they
  // spun with the camera, the avatar was forced upright, and the rotated resampling showed artifacts.)
  // Orientations are discrete, so this is always an exact quarter turn.
  (void)globalOrientation;
  // glm rotates CCW in y-up space which is clockwise on a y-down screen, matching SDL's convention
  return glm::degrees(object->getObjectOrientation().getAngleRadians());
}

std::vector<int32_t> SDLGridObserver::getExposedVariableValues(std::shared_ptr<ObjectRules> object) {
  std::vector<int32_t> variableValues;
  for (auto variableName : config_.shaderVariableConfig.exposedObjectVariables) {
    auto variableValuePtr = object->getVariableValue(variableName);
    if (variableValuePtr != nullptr) {
      variableValues.push_back(*variableValuePtr);
    } else {
      variableValues.push_back(0);
    }
  }
  return variableValues;
}

bool SDLGridObserver::supportsIncrementalItems() const {
  // Items of one cell must depend only on that cell (and its wall-tiling
  // neighbours): no camera that follows or rotates with an avatar, no
  // observer offsets or size overrides, no highlights that depend on other
  // observers, and an observer that implements appendObjectItems.
  if (getObserverType() != ObserverType::SPRITE_2D && getObserverType() != ObserverType::BLOCK_2D) return false;
  if (config_.trackAvatar || config_.rotateWithAvatar) return false;
  if (config_.gridXOffset != 0 || config_.gridYOffset != 0 || config_.overrideGridWidth != 0 || config_.overrideGridHeight != 0) return false;
  if (config_.highlightPlayers || config_.globalObserverAvatarMode != GlobalObserverAvatarMode::NONE) return false;
  return true;
}

void SDLGridObserver::appendItemsAt(glm::ivec2 location, std::vector<SDLRenderItem>& out) {
  if (location.x < 0 || location.y < 0 || location.x >= static_cast<int32_t>(gridWidth_) || location.y >= static_cast<int32_t>(gridHeight_)) return;
  // getObjectsAt is ordered by z, so items come out in draw order
  for (const auto& entry : grid_->getObjectsAt(location)) appendObjectItems(entry.second, out);
}

void SDLGridObserver::updateRenderItems() {
  // setOrientation reuses the existing storage; assigning a fresh
  // DiscreteOrientation would allocate two shared_ptr cells every frame
  globalOrientation_.setOrientation(avatarObject_ != nullptr && config_.rotateWithAvatar
                                        ? avatarObject_->getObjectOrientation().getDirection()
                                        : Direction::NONE);

  PartialObservableGrid observableGrid = getObservableGrid();

  if (avatarObject_ == nullptr || !avatarObject_->isRemoved()) {
    updateObjectRenderItems(observableGrid);
  }
}

}  // namespace griddly
