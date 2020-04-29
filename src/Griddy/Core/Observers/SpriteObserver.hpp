#include <memory>

#include "Vulkan/VulkanObserver.hpp"

namespace vk {
    struct SpriteData;
}

namespace griddy {

class SpriteObserver : public VulkanObserver {
 public:
  SpriteObserver(std::shared_ptr<Grid> grid, uint tileSize, std::unordered_map<std::string, std::string> spriteDesciptions);
  ~SpriteObserver();

  std::unique_ptr<uint8_t[]> update(int playerId) const override;
  std::unique_ptr<uint8_t[]> reset() const override;

  void init(uint gridWidth, uint gridHeight) override;
  

 private:
  vk::SpriteData loadImage(std::string imageFilename);

  void render(vk::VulkanRenderContext& ctx) const;
  std::unordered_map<std::string, std::string> spriteDesciptions_;
};

}  // namespace griddy