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

  std::shared_ptr<uint8_t> observation(new uint8_t[uniqueObjectCount * gridWidth * gridHeight]{});

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();

    if (observerConfig_.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getPartialObservableGrid(avatarLocation, avatarOrientation);
      uint32_t outx = 0, outy = 0;
      switch (avatarOrientation) {
        default:
        case Direction::DOWN:
        case Direction::NONE:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy++;
            }
            outx++;
          }
          break;
        case Direction::UP:
          outx = observerConfig_.gridWidth - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = observerConfig_.gridHeight - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outx = observerConfig_.gridWidth - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy++;
            }
            outx--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = observerConfig_.gridHeight - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy--;
            }
            outx++;
          }
          break;
      }

    } else {
      auto pGrid = getPartialObservableGrid(avatarLocation, Direction::NONE);

      uint32_t outx = 0, outy = 0;
      for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
        outy = 0;
        for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
          // place a 1 in every object "slice" where that object appears
          for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
            auto object = objectIt.second;
            int idx = uniqueObjectCount * (gridWidth * outy + outx) + object->getObjectId();
            observation.get()[idx] = 1;
          }
          outy++;
        }
        outx++;
      }
    }
  }

  return observation;
}

void VectorObserver::print(std::shared_ptr<uint8_t> observation) {
  auto gridWidth = observerConfig_.gridWidth;
  auto gridHeight = observerConfig_.gridHeight;

  std::string printString;

  // for (int h = height - 1; h >= 0; h--) {
  //   printString += "[";
  //   for (int w = 0; w < width; w++) {
  //     int idx = h * width + w;
  //     printString += " " + std::to_string(observation.get()[idx]) + " ";
  //   }
  //   printString += "]\n";
  // }
  spdlog::debug("TileObservation: \n {0}", printString);
}

}  // namespace griddly