#pragma once

#include <memory>

#include "../Grid.hpp"

namespace griddly {

enum class ObserverType { NONE,
                          VECTOR,
                          ENTITY,
                          ASCII,
#ifndef WASM
                          ISOMETRIC,
                          BLOCK_2D,
                          SPRITE_2D
#endif
};

struct ObserverConfig {
  uint32_t overrideGridWidth = 0;
  uint32_t overrideGridHeight = 0;
  int32_t gridXOffset = 0;
  int32_t gridYOffset = 0;
  bool rotateWithAvatar = false;
  bool trackAvatar = false;

  uint32_t playerId = 0;
  uint32_t playerCount = 1;
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
  explicit Observer(std::shared_ptr<Grid> grid, ObserverConfig& config);

  virtual void reset(std::shared_ptr<Object> avatarObject = nullptr);

  virtual void init(std::vector<std::weak_ptr<Observer>> playerObservers);

  PartialObservableGrid getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation = Direction::NONE) const;
  virtual PartialObservableGrid getObservableGrid() const;

  virtual bool trackAvatar() const;

  virtual void release();

  virtual ObserverType getObserverType() const = 0;

  // used to get the default observer for named observers
  static std::string getDefaultObserverName(ObserverType observerType);

  // Shuffle player Ids of objects so that current player is player 0
  uint32_t getEgocentricPlayerId(uint32_t objectPlayerId) const;

  virtual ~Observer() = default;

 protected:
  uint32_t gridHeight_ = 0;
  uint32_t gridWidth_ = 0;
  virtual void resetShape() = 0;

  // Boundary of the game grid regardless of render shape
  glm::ivec2 gridBoundary_;

  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Object> avatarObject_ = nullptr;

  ObserverState observerState_ = ObserverState::NONE;

  bool doTrackAvatar_ = false;
  std::vector<std::weak_ptr<Observer>> playerObservers_{};

 private:
  const ObserverConfig config_;
};
}  // namespace griddly
