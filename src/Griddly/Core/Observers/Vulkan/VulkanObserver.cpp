#include <spdlog/spdlog.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>
#include <memory>
#include <utility>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"
#include "VulkanObserver.hpp"

namespace griddly {

std::shared_ptr<vk::VulkanInstance> VulkanObserver::instance_ = nullptr;

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, VulkanObserverConfig& config) : Observer(std::move(grid), config) {
  config_ = config;
}

void VulkanObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  Observer::init(playerObservers);

  uint32_t playerCount = grid_->getPlayerCount();
  if (config_.playerColors.size() > 0) {
    if (config_.playerColors.size() >= playerCount) {
      for (const auto& playerColor : config_.playerColors) {
        playerColors_.emplace_back(glm::vec4(playerColor, 1.0));
      }
    } else {
      std::string error = fmt::format("The number of player colors is less than the number of players defined.");
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

  } else {
    float s = 1.0F;
    float v = 0.6F;
    float h_inc = 360.0F / playerCount;
    for (uint32_t p = 0; p < playerCount; p++) {
      uint32_t h = h_inc * p;
      glm::vec4 rgba = glm::vec4(glm::rgbColor(glm::vec3(h, s, v)), 1.0);
      playerColors_.push_back(rgba);
    }
  }
}

/**
 * Only load vulkan if update() called, allows many environments with vulkan-based global observers to be used.
 * But only loads them if global observations are requested, for example for creating videos
 *
 * This a) allows significantly more enviroments to be loaded (if only one of them is being used to create videos) and b)
 */
void VulkanObserver::lazyInit() {
  if (observerState_ != ObserverState::RESET) {
    throw std::runtime_error("Cannot initialize Vulkan Observer when it is not in RESET state.");
  }

  spdlog::debug("Vulkan lazy initialization....");

  gridBoundary_ = glm::ivec2(grid_->getWidth(), grid_->getHeight());

  auto imagePath = config_.resourceConfig.imagePath;
  auto shaderPath = config_.resourceConfig.shaderPath;

  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::make_shared<vk::VulkanInstance>(configuration);
  }

  device_ = std::make_shared<vk::VulkanDevice>(instance_, config_.tileSize, shaderPath);
  device_->initDevice(false);

  // This is probably far too big for most circumstances, but not sure how to work this one out in a smarter way,
  const int maxObjects = 100000;

  device_->initializeSSBOs(
      config_.shaderVariableConfig.exposedGlobalVariables.size(),
      grid_->getPlayerCount(),
      config_.shaderVariableConfig.exposedObjectVariables.size(),
      maxObjects);

  observerState_ = ObserverState::READY;
}

void VulkanObserver::reset(std::shared_ptr<Object> avatarObject) {
  Observer::reset(avatarObject);

  frameSSBOData_ = {};
  shouldUpdateCommandBuffer_ = true;

  if (observerState_ == ObserverState::READY) {
    resetRenderSurface();
  }
}

vk::PersistentSSBOData VulkanObserver::updatePersistentShaderBuffers() {
  spdlog::debug("Updating persistent shader buffers.");
  vk::PersistentSSBOData persistentSSBOData;

  spdlog::debug("Highlighting players {0}", config_.highlightPlayers ? "true" : "false");

  persistentSSBOData.environmentUniform.globalObserverAvatarMode = static_cast<uint32_t>(config_.globalObserverAvatarMode);
  persistentSSBOData.environmentUniform.viewMatrix = getViewMatrix();
  persistentSSBOData.environmentUniform.gridDims = glm::vec2{gridWidth_, gridHeight_};
  persistentSSBOData.environmentUniform.highlightPlayerObjects = config_.highlightPlayers ? 1 : 0;
  persistentSSBOData.environmentUniform.playerCount = config_.playerCount;
  persistentSSBOData.environmentUniform.playerId = config_.playerId;
  persistentSSBOData.environmentUniform.projectionMatrix = glm::ortho(0.0f, static_cast<float>(pixelWidth_), 0.0f, static_cast<float>(pixelHeight_));
  persistentSSBOData.environmentUniform.globalVariableCount = config_.shaderVariableConfig.exposedGlobalVariables.size();
  persistentSSBOData.environmentUniform.objectVariableCount = config_.shaderVariableConfig.exposedObjectVariables.size();

  return persistentSSBOData;
}

const glm::ivec2 VulkanObserver::getTileSize() const {
  return config_.tileSize;
}

uint8_t& VulkanObserver::update() {
  if (observerState_ == ObserverState::RESET) {
    lazyInit();
    resetRenderSurface();
  } else if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer is not in READY state, cannot render");
  }

  spdlog::debug("Updating frame shader buffers");
  updateFrameShaderBuffers();
  device_->writeFrameSSBOData(frameSSBOData_);

  if (shouldUpdateCommandBuffer_) {
    spdlog::debug("Writing command buffer");
    device_->startRecordingCommandBuffer();
    updateCommandBuffer();
    device_->endRecordingCommandBuffer(std::vector<VkRect2D>{{{0, 0}, {pixelWidth_, pixelHeight_}}});
    shouldUpdateCommandBuffer_ = false;
  }

  grid_->purgeUpdatedLocations(config_.playerId);

  return *device_->renderFrame();
}

void VulkanObserver::resetRenderSurface() {
  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}. Pixel width={2}. height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);
  observationStrides_ = device_->resetRenderSurface(pixelWidth_, pixelHeight_);

  auto persistentSSBOData = updatePersistentShaderBuffers();
  device_->writePersistentSSBOData(persistentSSBOData);
}

void VulkanObserver::release() {
  device_.reset();
}

}  // namespace griddly
