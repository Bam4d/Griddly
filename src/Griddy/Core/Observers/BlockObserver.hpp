#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace griddy {

class BlockObserver : public VulkanObserver {
 public:
  BlockObserver(std::shared_ptr<Grid> grid, uint32_t tileSize);
  ~BlockObserver();

  void init(uint gridWidth, uint gridHeight) override;

  std::unique_ptr<uint8_t[]> observe(int playerId) override;

 private:
};

}  // namespace griddy