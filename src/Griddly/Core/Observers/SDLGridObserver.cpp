#include "SDLGridObserver.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
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

void SDLGridObserver::reset(std::shared_ptr<ObjectRules> avatarObject) {
  SDLObserver::reset(avatarObject);
  // a new episode (or level) starts unexplored
  fogState_.clear();
  cameraOrigin_ = {0, 0};
}

int32_t SDLGridObserver::readGlobalVariable(const std::string& name, uint32_t playerId) const {
  const auto& globals = grid_->getGlobalVariables();
  auto it = globals.find(name);
  if (it == globals.end()) return 0;
  auto playerIt = it->second.find(playerId);
  if (playerIt == it->second.end()) playerIt = it->second.find(0);
  if (playerIt == it->second.end() || playerIt->second == nullptr) return 0;
  return *playerIt->second;
}

glm::ivec2 SDLGridObserver::windowOrigin(int32_t cx, int32_t cy, uint32_t width, uint32_t height) const {
  auto clampAxis = [](int32_t centre, uint32_t size, uint32_t mapSize) {
    int32_t origin = centre - static_cast<int32_t>(size / 2);
    int32_t maxOrigin = std::max(0, static_cast<int32_t>(mapSize) - static_cast<int32_t>(size));
    return std::clamp(origin, 0, maxOrigin);
  };
  return {clampAxis(cx, width, grid_->getWidth()), clampAxis(cy, height, grid_->getHeight())};
}

PartialObservableGrid SDLGridObserver::getObservableGrid() const {
  if (!usesCamera()) {
    return Observer::getObservableGrid();
  }
  const int32_t maxX = static_cast<int32_t>(grid_->getWidth()) - 1;
  const int32_t maxY = static_cast<int32_t>(grid_->getHeight()) - 1;
  PartialObservableGrid observableGrid;
  observableGrid.left = cameraOrigin_.x;
  observableGrid.right = std::min(cameraOrigin_.x + static_cast<int32_t>(gridWidth_) - 1, maxX);
  observableGrid.bottom = cameraOrigin_.y;
  observableGrid.top = std::min(cameraOrigin_.y + static_cast<int32_t>(gridHeight_) - 1, maxY);
  return observableGrid;
}

void SDLGridObserver::updateCamera() {
  if (!usesCamera()) return;
  cameraOrigin_ = windowOrigin(readGlobalVariable(config_.camera.xVariable, config_.playerId),
                               readGlobalVariable(config_.camera.yVariable, config_.playerId), gridWidth_, gridHeight_);
}

void SDLGridObserver::updateFog() {
  if (!usesFog()) return;
  const auto width = grid_->getWidth();
  const auto height = grid_->getHeight();
  const size_t cells = static_cast<size_t>(width) * height;
  // With Environment.Player.FogOfWar the engine tracks what each player sees (the same visibility
  // that limits the player's state queries); the observer shows exactly that.
  const auto& visibility = grid_->getVisibility();
  if (visibility.enabled() && visibility.cells(config_.playerId).size() == cells) {
    fogState_ = visibility.cells(config_.playerId);
    return;
  }
  // otherwise the observer keeps its own fog from its FogOfWar config
  if (fogState_.size() != cells) fogState_.assign(cells, UNSEEN);
  for (auto& cell : fogState_) {
    if (cell == VISIBLE) cell = EXPLORED;
  }
  PlayerVisibilityConfig config{true, config_.fogOfWar.sightVariable, config_.fogOfWar.defaultSight};
  for (const auto& object : grid_->getObjects()) {
    if (object->getPlayerId() != config_.playerId) continue;
    PlayerVisibility::reveal(fogState_, width, height, object->getLocation(), PlayerVisibility::sightOf(config, object));
  }
}

bool SDLGridObserver::fogHides(const std::shared_ptr<ObjectRules>& object) const {
  // Environment.Player.FogOfWar applies to every player observer, with or without a fog overlay
  if (!playerCanSee(object)) return true;
  if (!usesFog() || fogState_.empty()) return false;
  const auto location = object->getLocation();
  if (location.x < 0 || location.y < 0 || location.x >= static_cast<int32_t>(grid_->getWidth()) || location.y >= static_cast<int32_t>(grid_->getHeight())) {
    return false;
  }
  const auto state = fogState_[static_cast<size_t>(location.y) * grid_->getWidth() + location.x];
  if (state == VISIBLE) return false;
  // other players' objects are only seen while in sight; terrain and neutral objects stay on the map
  // once explored (never-seen cells are covered by the fog itself)
  const auto owner = object->getPlayerId();
  return owner != 0 && owner != config_.playerId;
}

void SDLGridObserver::appendHealthBar(const std::shared_ptr<ObjectRules>& object, glm::vec2 center, std::vector<SDLRenderItem>& out) {
  if (!config_.healthBars.enabled) return;
  auto healthVariable = object->getVariableValue(config_.healthBars.healthVariable);
  auto maxHealthVariable = object->getVariableValue(config_.healthBars.maxHealthVariable);
  if (healthVariable == nullptr || maxHealthVariable == nullptr) return;
  const int32_t health = *healthVariable;
  const int32_t maxHealth = *maxHealthVariable;
  if (maxHealth <= 0 || (health >= maxHealth && !config_.healthBars.always)) return;
  auto* texture = getSpriteTexture(kSolidSprite);
  if (texture == nullptr) return;

  const auto tileSize = glm::vec2(config_.tileSize);
  const float barWidth = std::round(tileSize.x * 0.8f);
  const float barHeight = std::max(2.0f, std::round(tileSize.y / 8.0f));
  const float x = std::round(center.x - barWidth / 2.0f);
  const float y = std::round(center.y - tileSize.y / 2.0f) + 1.0f;
  const float fraction = std::clamp(static_cast<float>(health) / static_cast<float>(maxHealth), 0.0f, 1.0f);
  const float z = static_cast<float>(object->getZIdx()) + 0.75f;

  SDLRenderItem background;
  background.texture = texture;
  background.src = {0, 0, 4, 4};
  background.dst = {x, y, barWidth, barHeight};
  background.r = background.g = background.b = 0;
  background.a = 190;
  background.z = z;
  background.sortY = center.y;
  out.push_back(background);

  SDLRenderItem bar = background;
  bar.dst.w = std::max(1.0f, std::round(barWidth * fraction));
  if (fraction > 0.6f) {
    bar.r = 74, bar.g = 222, bar.b = 128;
  } else if (fraction > 0.3f) {
    bar.r = 250, bar.g = 204, bar.b = 21;
  } else {
    bar.r = 248, bar.g = 113, bar.b = 113;
  }
  bar.a = 255;
  bar.z = z + 0.01f;
  out.push_back(bar);
}

void SDLGridObserver::appendFogItems(const PartialObservableGrid& observableGrid) {
  if (!usesFog() || fogState_.empty()) return;
  auto* texture = getSpriteTexture(kSolidSprite);
  if (texture == nullptr) return;
  const auto& fog = config_.fogOfWar;
  const auto width = grid_->getWidth();
  const auto tileSize = glm::vec2(config_.tileSize);
  const auto colorR = static_cast<uint8_t>(fog.color.r * 255.0f);
  const auto colorG = static_cast<uint8_t>(fog.color.g * 255.0f);
  const auto colorB = static_cast<uint8_t>(fog.color.b * 255.0f);
  const auto exploredAlpha = static_cast<uint8_t>(std::clamp(fog.exploredOpacity, 0.0f, 1.0f) * 255.0f);
  const auto unexploredAlpha = static_cast<uint8_t>(std::clamp(fog.unexploredOpacity, 0.0f, 1.0f) * 255.0f);

  // one item per horizontal run of cells in the same fog state
  for (int32_t y = observableGrid.bottom; y <= observableGrid.top; y++) {
    int32_t x = observableGrid.left;
    while (x <= observableGrid.right) {
      const auto state = fogState_[static_cast<size_t>(y) * width + x];
      int32_t end = x;
      while (end + 1 <= observableGrid.right && fogState_[static_cast<size_t>(y) * width + end + 1] == state) end++;
      if (state != VISIBLE) {
        const auto first = getTilePixelCenter({x, y});
        SDLRenderItem item;
        item.texture = texture;
        item.src = {0, 0, 4, 4};
        item.dst = {first.x - tileSize.x / 2.0f, first.y - tileSize.y / 2.0f, tileSize.x * static_cast<float>(end - x + 1), tileSize.y};
        item.r = colorR, item.g = colorG, item.b = colorB;
        item.a = state == EXPLORED ? exploredAlpha : unexploredAlpha;
        item.z = 1000.0f;
        item.sortY = first.y;
        renderItems_.push_back(item);
      }
      x = end + 1;
    }
  }
}

void SDLGridObserver::appendCameraFrames() {
  if (!config_.cameraFrame.enabled) return;
  auto* texture = getSpriteTexture(kSolidSprite);
  if (texture == nullptr) return;
  const auto& frame = config_.cameraFrame;
  const auto tileSize = glm::vec2(config_.tileSize);
  const float thickness = std::max(1.0f, std::round(std::min(tileSize.x, tileSize.y) / 4.0f));
  for (uint32_t playerId = 1; playerId <= config_.playerCount; playerId++) {
    // a player's own observer only shows its own camera
    if (config_.playerId != 0 && playerId != config_.playerId) continue;
    auto origin = windowOrigin(readGlobalVariable(frame.xVariable, playerId), readGlobalVariable(frame.yVariable, playerId), frame.width, frame.height);
    const auto w = std::min(frame.width, grid_->getWidth());
    const auto h = std::min(frame.height, grid_->getHeight());
    const auto topLeft = getTilePixelCenter(origin) - tileSize / 2.0f;
    const auto bottomRight = getTilePixelCenter(origin + glm::ivec2(w - 1, h - 1)) + tileSize / 2.0f;
    auto colorId = getEgocentricPlayerId(playerId);
    glm::vec4 color = (colorId >= 1 && colorId <= playerColors_.size()) ? playerColors_[colorId - 1] : glm::vec4(1.0f);
    const SDL_FRect edges[4] = {
        {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, thickness},
        {topLeft.x, bottomRight.y - thickness, bottomRight.x - topLeft.x, thickness},
        {topLeft.x, topLeft.y, thickness, bottomRight.y - topLeft.y},
        {bottomRight.x - thickness, topLeft.y, thickness, bottomRight.y - topLeft.y},
    };
    for (const auto& edge : edges) {
      SDLRenderItem item;
      item.texture = texture;
      item.src = {0, 0, 4, 4};
      item.dst = edge;
      item.r = static_cast<uint8_t>(color.r * 255.0f);
      item.g = static_cast<uint8_t>(color.g * 255.0f);
      item.b = static_cast<uint8_t>(color.b * 255.0f);
      item.z = 1001.0f;
      renderItems_.push_back(item);
    }
  }
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
  // RGBA8 interleaved, tightly packed (see SDLObserver::resetRenderSurface)
  observationStrides_ = {1, 4, pixelWidth_ * 4};
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

  if (usesCamera()) {
    tilePos -= glm::vec2(cameraOrigin_);
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
  // camera windows, fog, health bars and camera frames depend on more than the cell's own objects
  if (config_.camera.enabled || config_.fogOfWar.enabled || config_.healthBars.enabled || config_.cameraFrame.enabled) return false;
  if (grid_->getVisibility().enabled() && config_.playerId != 0) return false;
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

  updateCamera();
  updateFog();

  PartialObservableGrid observableGrid = getObservableGrid();

  if (avatarObject_ == nullptr || !avatarObject_->isRemoved()) {
    updateObjectRenderItems(observableGrid);
  }

  appendFogItems(observableGrid);
  appendCameraFrames();
}

}  // namespace griddly
