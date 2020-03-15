#include "Vulkan/VulkanObserver.hpp"

namespace griddy {

class BlockObserver : public VulkanObserver {
 public:
  std::unique_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) override;

  void print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) override;

 private:
}

}  // namespace griddy