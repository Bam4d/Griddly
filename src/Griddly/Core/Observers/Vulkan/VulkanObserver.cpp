#include "VulkanObserver.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace griddly {

std::shared_ptr<vk::VulkanInstance> VulkanObserver::instance_ = nullptr;

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig) : Observer(grid), resourceConfig_(resourceConfig) {
}

VulkanObserver::~VulkanObserver() {
}

void VulkanObserver::init(ObserverConfig observerConfig) {
  observerConfig_ = observerConfig;
  auto imagePath = resourceConfig_.imagePath;
  auto shaderPath = resourceConfig_.shaderPath;

  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<vk::VulkanInstance>(new vk::VulkanInstance(configuration));
  }

  std::unique_ptr<vk::VulkanDevice> vulkanDevice(new vk::VulkanDevice(instance_, observerConfig.tileSize, shaderPath));

  device_ = std::move(vulkanDevice);

  device_->initDevice(false);
  resetShape();
}

void VulkanObserver::resetShape() {
  resetRenderSurface();
}

std::shared_ptr<uint8_t> VulkanObserver::reset() {
  resetShape();

  auto ctx = device_->beginRender();

  render(ctx);

  // Only update the rectangles that have changed to save bandwidth/processing speed
  std::vector<VkRect2D> dirtyRectangles = {
      {{0, 0},
       {pixelWidth_, pixelHeight_}}};

  return device_->endRender(ctx, dirtyRectangles);
}

std::shared_ptr<uint8_t> VulkanObserver::update() const {
  auto ctx = device_->beginRender();

  render(ctx);

  std::vector<VkRect2D> dirtyRectangles;

  // Optimize this in the future, partial observation is slower for the moment
  if (avatarObject_ != nullptr) {
    std::vector<VkRect2D> dirtyRectangles = {
        {{0, 0},
         {pixelWidth_, pixelHeight_}}};

    return device_->endRender(ctx, dirtyRectangles);
  }

  dirtyRectangles = calculateDirtyRectangles(grid_->getUpdatedLocations());

  return device_->endRender(ctx, dirtyRectangles);
}

void VulkanObserver::resetRenderSurface() {
  // Delete old render surfaces (if they exist)

  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto tileSize = observerConfig_.tileSize;

  pixelWidth_ = gridWidth_ * tileSize.x;
  pixelHeight_ = gridHeight_ * tileSize.y;

  observationShape_ = {3, pixelWidth_, pixelHeight_};
  observationStrides_ = {1, 3, 3 * pixelWidth_};

  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}. Pixel width={2}. height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);

  device_->resetRenderSurface(pixelWidth_, pixelHeight_);
}

void VulkanObserver::release() {
  device_.reset();
}

void VulkanObserver::print(std::shared_ptr<uint8_t> observation) {
  auto tileSize = observerConfig_.tileSize;
  std::string filename = fmt::format("{0}.ppm", *grid_->getTickCount());
  std::ofstream file(filename, std::ios::out | std::ios::binary);

  auto width = grid_->getWidth() * tileSize.x;
  auto height = grid_->getHeight() * tileSize.y;

  // ppm header
  file << "P6\n"
       << width << "\n"
       << height << "\n"
       << 255 << "\n";

  file.write((char*)observation.get(), width * height * 3);
  file.close();
}

}  // namespace griddly