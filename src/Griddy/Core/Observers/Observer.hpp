#pragma once 

#include <memory>
#include "../Grid.hpp"

namespace griddy {
class Observer {
 public:
  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual std::unique_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) = 0;

  virtual void print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) = 0;

 private:
};
}  // namespace griddy