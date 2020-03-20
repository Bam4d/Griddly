#include "VulkanObserver.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace griddy {

VulkanObserver::VulkanObserver(int tileSize) : tileSize_(tileSize) {
}

VulkanObserver::~VulkanObserver() {}

void VulkanObserver::init(int gridWidth, int gridHeight) {
  auto configuration = vk::VulkanConfiguration();
  std::unique_ptr<vk::VulkanInstance> vulkanInstance(new vk::VulkanInstance(configuration));

  auto width = gridWidth * tileSize_;
  auto height = gridHeight * tileSize_;

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(std::move(vulkanInstance), width, height));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
}

std::unique_ptr<uint8_t[]> VulkanObserver::observe(int playerId, std::shared_ptr<Grid> grid) {
  auto width = grid->getWidth();
  auto height = grid->getHeight();

  auto ctx = device_->beginRender();

  device_->drawTriangle(ctx, {0.0, 0.0, 0.0});
  device_->drawTriangle(ctx, {0.0, 1.0, 0.0});
  device_->drawTriangle(ctx, {1.0, 1.0, 0.0});

  return device_->endRender(ctx);
}

void VulkanObserver::print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) {
  std::string filename = fmt::format("{0}.ppm", grid->getTickCount());
  std::ofstream file(filename, std::ios::out | std::ios::binary);

  auto width = grid->getWidth() * tileSize_;
  auto height = grid->getHeight() * tileSize_;

  // ppm header
  file << "P6\n"
       << width << "\n"
       << height << "\n"
       << 255 << "\n";

  file.write((char*)observation.get(), width*height*3);
  file.close();
}

}  // namespace griddy