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
  auto imagePath = vulkanObserverConfig_.imagePath;
  auto shaderPath = vulkanObserverConfig_.shaderPath;
  
  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<vk::VulkanInstance>(new vk::VulkanInstance(configuration));
  }

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(instance_, shaderPath));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
  
  resetRenderSurface();
}

void VulkanObserver::resetRenderSurface() {
  // Delete old render surfaces (if they exist)

  auto gridWidth = grid_->getWidth();
  auto gridHeight = grid_->getHeight();

  auto tileSize = vulkanObserverConfig_.tileSize;

  uint32_t pixelWidth = gridWidth * tileSize;
  uint32_t pixelHeight = gridHeight * tileSize;

  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}, tileSize={2}", gridWidth, gridHeight, tileSize);

  device_->resetRenderSurface(pixelWidth, pixelHeight, tileSize);
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