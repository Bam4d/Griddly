#pragma once

#include <memory>

#include "../Grid.hpp"

namespace griddly {

struct ObserverConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  int32_t gridXOffset;
  int32_t gridYOffset;
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

  virtual void init(ObserverConfig observerConfig);
  virtual void setAvatar(std::shared_ptr<Object> avatarObject);

  virtual void print(std::shared_ptr<uint8_t> observation);

  virtual ~Observer() = 0;

 protected:
  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Object> avatarObject_;
  ObserverConfig observerConfig_;
};
}  // namespace griddly