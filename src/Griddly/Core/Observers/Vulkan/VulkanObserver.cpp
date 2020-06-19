#include "VulkanObserver.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace griddly {

std::shared_ptr<vk::VulkanInstance> VulkanObserver::instance_ = nullptr;

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig vulkanObserverConfig) : Observer(grid), vulkanObserverConfig_(vulkanObserverConfig) {
}

VulkanObserver::~VulkanObserver() {
}

void VulkanObserver::init(ObserverConfig observerConfig) {
  Observer::init(observerConfig);
  auto tileSize = vulkanObserverConfig_.tileSize;
  auto imagePath = vulkanObserverConfig_.imagePath;
  auto shaderPath = vulkanObserverConfig_.shaderPath;
  auto gridWidth = observerConfig_.gridWidth;
  auto gridHeight = observerConfig_.gridHeight;

  spdlog::debug("Initializing Vulkan Observer. Grid width={0}, height={1}, tileSize={2}", observerConfig_.gridWidth, observerConfig_.gridHeight, tileSize);
  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<vk::VulkanInstance>(new vk::VulkanInstance(configuration));
  }

  uint32_t pixelWidth = gridWidth * tileSize;
  uint32_t pixelHeight = gridHeight * tileSize;

  observationShape_ = {3, pixelWidth, pixelHeight};
  observationStrides_ = {1, 3, 3 * pixelWidth};

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(instance_, pixelWidth, pixelHeight, tileSize, shaderPath));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
}

void VulkanObserver::release() {
  device_.reset();
}

void VulkanObserver::print(std::shared_ptr<uint8_t> observation) {
  auto tileSize = vulkanObserverConfig_.tileSize;
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