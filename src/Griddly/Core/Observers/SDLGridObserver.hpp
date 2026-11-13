#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "SDL/SDLObserver.hpp"

namespace griddly {

struct SDLGridObserverConfig : public SDLObserverConfig {
  bool rotateAvatarImage = true;
};

class SDLGridObserver : public SDLObserver {
 public:
  explicit SDLGridObserver(std::shared_ptr<Grid> grid, SDLGridObserverConfig& config);
  ~SDLGridObserver() override = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;
  void reset(std::shared_ptr<ObjectRules> avatarObject = nullptr) override;

  PartialObservableGrid getObservableGrid() const override;

  // Fog of war state of every map cell for this observer's player (row major, map width x height):
  // 0 = never seen, 1 = explored (seen before), 2 = visible now. Empty without FogOfWar.
  const std::vector<uint8_t>& fogState() const { return fogState_; }
  // Top-left map cell shown by a Camera observer (0,0 otherwise)
  glm::ivec2 cameraOrigin() const { return cameraOrigin_; }
  // Size of the rendered view in cells
  glm::uvec2 viewCells() const { return {gridWidth_, gridHeight_}; }

 protected:
  void updateRenderItems() override;
  virtual void updateObjectRenderItems(PartialObservableGrid& partiallyObservableGrid) = 0;
  // Render items of one object (used to rebuild single cells incrementally);
  // false from supportsIncrementalItems() means it is not implemented
  virtual void appendObjectItems(const std::shared_ptr<ObjectRules>& object, std::vector<SDLRenderItem>& out) { (void)object; (void)out; }
  bool supportsIncrementalItems() const override;
  void appendItemsAt(glm::ivec2 location, std::vector<SDLRenderItem>& out) override;

  void resetShape() override;

  std::vector<int32_t> getExposedVariableValues(std::shared_ptr<ObjectRules> object);

  // Pixel-space center of a grid tile, applying offsets and avatar tracking/rotation
  virtual glm::vec2 getTilePixelCenter(glm::ivec2 location) const;
  // Rotation applied to object sprites (degrees, SDL clockwise convention)
  double getObjectAngle(std::shared_ptr<ObjectRules> object, DiscreteOrientation globalOrientation) const;

  // Value of a global variable for a player (per-player variables), or its shared value
  int32_t readGlobalVariable(const std::string& name, uint32_t playerId) const;
  // Top-left cell of a width x height window centred on (cx, cy), clamped to the map
  glm::ivec2 windowOrigin(int32_t cx, int32_t cy, uint32_t width, uint32_t height) const;
  bool usesCamera() const { return config_.camera.enabled && !doTrackAvatar_; }
  bool usesFog() const { return config_.fogOfWar.enabled && config_.playerId != 0; }
  void updateCamera();
  void updateFog();
  // true when fog of war hides this object from the observing player
  bool fogHides(const std::shared_ptr<ObjectRules>& object) const;
  void appendHealthBar(const std::shared_ptr<ObjectRules>& object, glm::vec2 center, std::vector<SDLRenderItem>& out);
  void appendFogItems(const PartialObservableGrid& observableGrid);
  void appendCameraFrames();
  bool needsSolidTexture() const { return config_.fogOfWar.enabled || config_.healthBars.enabled || config_.cameraFrame.enabled; }

  // 4x4 white texture tinted for fog, health bars and camera frames
  static constexpr const char* kSolidSprite = "__solid";

  SDLGridObserverConfig config_;
  DiscreteOrientation globalOrientation_;
  std::vector<uint8_t> fogState_;
  glm::ivec2 cameraOrigin_{0, 0};
};

}  // namespace griddly
