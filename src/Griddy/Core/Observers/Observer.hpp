#pragma once

#include <memory>

#include "../Grid.hpp"

namespace griddy {
class Observer {
 public:
  Observer(std::shared_ptr<Grid> grid);

  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual std::unique_ptr<uint8_t[]> update(int playerId) const = 0;

  virtual std::unique_ptr<uint8_t[]> reset() const = 0;

  virtual std::vector<uint32_t> getShape() const = 0;
  virtual std::vector<uint32_t> getStrides() const = 0;

  virtual void init(uint32_t gridWidth, uint32_t gridHeight);

  

  virtual void print(std::unique_ptr<uint8_t[]> observation);

  virtual ~Observer() = 0;

 protected:
  const std::shared_ptr<Grid> grid_;
};
}  // namespace griddy