#pragma once

#include <memory>

#include "../Grid.hpp"

namespace griddly {

struct ObserverConfig {
  bool trackAvatar;
  uint32_t width;
  uint32_t height;
  int32_t xOffset;
  int32_t yOffset;
};

struct PartialObservationRect {
  uint32_t top;
  uint32_t bottom;
  uint32_t left;
  uint32_t right;
};

class Observer {
 public:
  Observer(std::shared_ptr<Grid> grid);

  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual std::shared_ptr<uint8_t> update(int playerId) const = 0;

  virtual std::shared_ptr<uint8_t> reset() const = 0;

  virtual std::vector<uint32_t> getShape() const = 0;
  virtual std::vector<uint32_t> getStrides() const = 0;

  PartialObservationRect getPartialObservationRect() const;

  virtual void setAvatar(std::shared_ptr<Object> avatarObject);

  virtual void init(ObserverConfig observerConfig);

  virtual void print(std::shared_ptr<uint8_t> observation);

  virtual ~Observer() = 0;

 protected:
  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Object> avatarObject_;
  ObserverConfig observerConfig_;
};
}  // namespace griddly