#include <spdlog/spdlog.h>
#include "VulkanObserver.hpp"
#include "VulkanConfiguration.hpp"
#include "VulkanInstance.hpp"

namespace griddy {

VulkanObserver::VulkanObserver(int height, int width) : height_(height), width_(width) {
  auto configuration = vk::VulkanConfiguration();
  auto instance = vk::VulkanInstance(configuration);

}

VulkanObserver::~VulkanObserver() {}

std::unique_ptr<uint8_t[]> VulkanObserver::observe(int playerId, std::shared_ptr<Grid> grid) {
  int width = grid->getWidth();
  int height = grid->getHeight();

  std::unique_ptr<uint8_t[]> observation(new uint8_t[width * height]{});

  return std::move(observation);
}

void VulkanObserver::print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) {
  spdlog::debug("Vulcan Observation");
}

}  // namespace griddy