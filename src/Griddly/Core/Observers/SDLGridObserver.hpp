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

 protected:
  void updateRenderItems() override;
  virtual void updateObjectRenderItems(PartialObservableGrid& partiallyObservableGrid) = 0;

  void resetShape() override;

  std::vector<int32_t> getExposedVariableValues(std::shared_ptr<ObjectRules> object);

  // Pixel-space center of a grid tile, applying offsets and avatar tracking/rotation
  virtual glm::vec2 getTilePixelCenter(glm::ivec2 location) const;
  // Rotation applied to object sprites (degrees, SDL clockwise convention)
  double getObjectAngle(std::shared_ptr<ObjectRules> object, DiscreteOrientation globalOrientation) const;

  SDLGridObserverConfig config_;
  DiscreteOrientation globalOrientation_;
};

}  // namespace griddly
