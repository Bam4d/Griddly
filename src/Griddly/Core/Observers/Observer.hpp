#pragma once

#include <memory>

#include "../Grid.hpp"

namespace griddly {

struct ObserverConfig {
  uint32_t overrideGridWidth = 0;
  uint32_t overrideGridHeight = 0;
  int32_t gridXOffset = 0;
  int32_t gridYOffset = 0;
  bool rotateWithAvatar = false;
  
  uint32_t playerId = 0;
  uint32_t playerCount = 1;
  

  // // Config for VECTOR observers only
  // bool includeVariables = false;
  // bool includeRotation = false;
  // bool includePlayerId = false;

  // // Config for ASCII observers
  // uint32_t asciiPadWidth = 4;

  // // Config for Isometric observers
  // uint32_t isoTileDepth = 0;
  // uint32_t isoTileHeight = 0;

  // // Config for observers that use sprites
};

struct PartialObservableGrid {
  int32_t top;
  int32_t bottom;
  int32_t left;
  int32_t right;
};

enum class ObserverState {
  NONE,
  INITIALISED,
  RESET,
  READY
};

class Observer {
 public:
  explicit Observer(std::shared_ptr<Grid> grid);

  virtual void reset();

  void init(int32_t gridXOffset, int32_t gridYOffset);

  PartialObservableGrid getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation=Direction::NONE) const;
  
  virtual void setAvatar(std::shared_ptr<Object> avatarObject);

  virtual void release();

  virtual ~Observer() = default;

 protected:
  uint32_t gridWidth_;
  uint32_t gridHeight_;

  int32_t gridXOffset_; 
  int32_t gridYOffset_;

  virtual void resetShape() = 0;
  void init();

  // Boundary of the game grid regardless of render shape
  glm::ivec2 gridBoundary_;

  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Object> avatarObject_;

  ObserverState observerState_ = ObserverState::NONE;
};
}  // namespace griddly