#include <vulkan/vulkan.h>
#include "../../Grid.hpp"
#include "../Observer.hpp"

namespace griddy {

class VulkanObserver : public Observer {
 public:
  VulkanObserver(int height, int width);

  ~VulkanObserver();

  std::unique_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) override;

  void print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) override;

 private:

  const int height_;
  const int width_;
};

}  // namespace griddy