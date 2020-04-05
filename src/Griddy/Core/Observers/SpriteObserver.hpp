#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddy {

class SpriteObserver : public VulkanObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, uint tileSize);
  ~SpriteObserver();

  std::unique_ptr<uint8_t[]> observe(int playerId) override;

  void init(uint gridWidth, uint gridHeight) override;

 private:
};

}  // namespace griddy