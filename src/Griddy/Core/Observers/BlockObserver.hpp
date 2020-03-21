#include "Vulkan/VulkanObserver.hpp"
#include <memory>

namespace griddy {

class BlockObserver : public VulkanObserver {
 public:
  BlockObserver(uint32_t tileSize);
  ~BlockObserver();

  std::unique_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) override;

 private:
};

}  // namespace griddy