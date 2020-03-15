#include <spdlog/spdlog.h>
#include "VulkanObserver.hpp"
#include "VulkanConfiguration.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDevice.hpp"

namespace griddy {

VulkanObserver::VulkanObserver(int tileSize): tileSize_(tileSize) {
}

VulkanObserver::~VulkanObserver() {}

void VulkanObserver::init(int gridWidth, int gridHeight) {
  auto configuration = vk::VulkanConfiguration();
  std::unique_ptr<vk::VulkanInstance> vulkanInstance(new vk::VulkanInstance(configuration));

  auto width = gridWidth * tileSize_;
  auto height = gridHeight * tileSize_;

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(std::move(vulkanInstance), width, height));

  device_ = std::move(vulkanDevice);
  
  device_->initDevice(true);
}

std::unique_ptr<uint8_t[]> VulkanObserver::observe(int playerId, std::shared_ptr<Grid> grid) {
  int width = grid->getWidth();
  int height = grid->getHeight();

  auto ctx = device_->beginRender();

  device_->drawSquare(ctx, {0.0, 0.0, 0.0});
  device_->drawSquare(ctx, {0.0, 1.0, 0.0});
  device_->drawSquare(ctx, {1.0, 1.0, 0.0});

  return std::move(device_->endRender(ctx));
}

}  // namespace griddy