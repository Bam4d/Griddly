#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace vk {
    struct SpriteData;
}

namespace griddy {

class SpriteObserver : public VulkanObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, uint tileSize);
  ~SpriteObserver();

  std::unique_ptr<uint8_t[]> observe(int playerId) const override;

  void init(uint gridWidth, uint gridHeight) override;

 private:
  vk::SpriteData loadImage(std::string imageFilename);
};

}  // namespace griddy