#include "VulkanObserver.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace griddly {

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, ObserverConfig observerConfig) : Observer(grid), observerConfig_(observerConfig) {
}

VulkanObserver::~VulkanObserver() {}

void VulkanObserver::init(uint32_t gridWidth, uint32_t gridHeight) {
  auto tileSize = observerConfig_.tileSize;
  auto resourcePath = observerConfig_.resourcePath;
  spdlog::debug("Initializing Vulkan Observer. Grid width={0}, height={1}, tileSize={2}", gridWidth, gridHeight, tileSize);
  auto configuration = vk::VulkanConfiguration();
  std::unique_ptr<vk::VulkanInstance> vulkanInstance(new vk::VulkanInstance(configuration));


  uint32_t width, height;
  if (observerConfig_.isPartialObserver) {
    width = observerConfig_.partialObserverConfig.width * tileSize;
    height = observerConfig_.partialObserverConfig.height * tileSize;
  } else {
    width = gridWidth * tileSize;
    height = gridHeight * tileSize;

    observationShape_ = {3, width, height};
    observationStrides_ = {1, 3, 3 * width};
  }

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(std::move(vulkanInstance), width, height, tileSize, resourcePath));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
}

std::vector<uint32_t> VulkanObserver::getShape() const {
  return observationShape_;
}

std::vector<uint32_t> VulkanObserver::getStrides() const {
  return observationStrides_;
}

void VulkanObserver::print(std::shared_ptr<uint8_t> observation) {
  auto tileSize = observerConfig_.tileSize;
  std::string filename = fmt::format("{0}.ppm", *grid_->getTickCount());
  std::ofstream file(filename, std::ios::out | std::ios::binary);

  auto width = grid_->getWidth() * tileSize;
  auto height = grid_->getHeight() * tileSize;

  // ppm header
  file << "P6\n"
       << width << "\n"
       << height << "\n"
       << 255 << "\n";

  file.write((char*)observation.get(), width * height * 3);
  file.close();
}

}  // namespace griddly