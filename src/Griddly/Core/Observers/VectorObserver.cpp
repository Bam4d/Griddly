#include "VectorObserver.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace griddly {

VectorObserver::VectorObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}

VectorObserver::~VectorObserver() {}

void VectorObserver::init(ObserverConfig observerConfig) {
  Observer::init(observerConfig);

  auto gridWidth = observerConfig_.gridWidth;
  auto gridHeight = observerConfig_.gridHeight;
  auto uniqueObjectCount = grid_->getUniqueObjectCount();

  observationShape_ = {uniqueObjectCount, gridWidth, gridHeight};
  observationStrides_ = {1, uniqueObjectCount, uniqueObjectCount * gridWidth};
}

std::shared_ptr<uint8_t> VectorObserver::reset() const {
  return update(0);
};

std::shared_ptr<uint8_t> VectorObserver::update(int playerId) const {
  auto gridWidth = observerConfig_.gridWidth;
  auto gridHeight = observerConfig_.gridHeight;
  auto uniqueObjectCount = grid_->getUniqueObjectCount();

  std::shared_ptr<uint8_t> observation(new uint8_t[gridWidth * gridHeight * uniqueObjectCount]{});

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();

    if (observerConfig_.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getPartialObservableGrid(avatarLocation, avatarOrientation);

      for (auto object : grid_->getObjects()) {
        int x = object->getLocation().x;
        int y = object->getLocation().y;

        int idx = gridWidth * y + x;

        observation.get()[idx] = object->getObjectId();
      }
    } else {
      for (auto object : grid_->getObjects()) {
        int x = object->getLocation().x;
        int y = object->getLocation().y;

        if (observerConfig_.rotateWithAvatar)
          int idx = gridWidth * y + x;

        observation.get()[idx] = object->getObjectId();
      }

      return observation;
    }

    void VectorObserver::print(std::shared_ptr<uint8_t> observation) {
      auto gridWidth = observerConfig_.gridWidth;
      auto gridHeight = observerConfig_.gridHeight;

      std::string printString;

      for (int h = height - 1; h >= 0; h--) {
        printString += "[";
        for (int w = 0; w < width; w++) {
          int idx = h * width + w;
          printString += " " + std::to_string(observation.get()[idx]) + " ";
        }
        printString += "]\n";
      }
      spdlog::debug("TileObservation: \n {0}", printString);
    }

  }  // namespace griddly