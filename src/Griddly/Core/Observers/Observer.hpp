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
  uint32_t isoTileDepth = 0;
  uint32_t isoTileHeight = 0;
  glm::ivec2 tileSize = {24, 24};
};

struct PartialObservableGrid {
  int32_t top;
  int32_t bottom;
  int32_t left;
  int32_t right;
};

enum class ObserverType { NONE,
                          SPRITE_2D,
                          BLOCK_2D,
                          ISOMETRIC,
                          VECTOR };

class Observer {
 public:
  Observer(std::shared_ptr<Grid> grid);

  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual std::shared_ptr<uint8_t> update() const = 0;
  virtual std::shared_ptr<uint8_t> reset() = 0;
  
  virtual std::vector<uint32_t> getShape() const;
  virtual std::vector<uint32_t> getStrides() const;

  virtual glm::ivec2 getTileSize() const;

  virtual PartialObservableGrid getAvatarObservableGrid(glm::ivec2 avatarLocation, Direction avatarOrientation) const;

  virtual void init(ObserverConfig observerConfig);
  virtual void setAvatar(std::shared_ptr<Object> avatarObject);

  virtual void print(std::shared_ptr<uint8_t> observation);

  virtual void release();

  virtual ObserverType getObserverType() const = 0;

  virtual ~Observer() = 0;

 protected:
  uint32_t gridWidth_;
  uint32_t gridHeight_;

  virtual void resetShape() = 0;

  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Object> avatarObject_;
  ObserverConfig observerConfig_;
  std::vector<uint32_t> observationShape_;
  std::vector<uint32_t> observationStrides_;
};
}  // namespace griddly