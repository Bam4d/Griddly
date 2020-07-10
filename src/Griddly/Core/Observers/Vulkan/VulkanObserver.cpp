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

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(instance_, vulkanObserverConfig_.tileSize, shaderPath));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
}

void VulkanObserver::resetRenderSurface() {
  // Delete old render surfaces (if they exist)

  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto tileSize = vulkanObserverConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize;
  pixelHeight_ = gridHeight_ * tileSize;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
  observationStrides_ = {1, 3, 3 * pixelWidth_};

  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}, tileSize={2}", gridWidth_, gridHeight_, tileSize);

  device_->resetRenderSurface(pixelWidth_, pixelHeight_);
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