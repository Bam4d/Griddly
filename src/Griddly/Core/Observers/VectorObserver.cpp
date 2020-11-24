#include "VectorObserver.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace griddly {

VectorObserver::VectorObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}

VectorObserver::~VectorObserver() {}

void VectorObserver::init(ObserverConfig observerConfig) {
  Observer::init(observerConfig);
}

ObserverType VectorObserver::getObserverType() const {
 return ObserverType::VECTOR; 
}

glm::ivec2 VectorObserver::getTileSize() const {
  return glm::ivec2{1,1};
}

std::shared_ptr<uint8_t> VectorObserver::reset() {
  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  auto uniqueObjectCount = grid_->getUniqueObjectCount();

  observationShape_ = {uniqueObjectCount, gridWidth_, gridHeight_};
  observationStrides_ = {1, uniqueObjectCount, uniqueObjectCount * gridWidth_};

  return update();
};

std::shared_ptr<uint8_t> VectorObserver::update() const {
  auto uniqueObjectCount = grid_->getUniqueObjectCount();

  std::shared_ptr<uint8_t> observation(new uint8_t[uniqueObjectCount * gridWidth_ * gridHeight_]{});

  if (avatarObject_ != nullptr) {
    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarOrientation.getDirection();

    if (observerConfig_.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getAvatarObservableGrid(avatarLocation, avatarDirection);
      uint32_t outx = 0, outy = 0;
      switch (avatarDirection) {
        default:
        case Direction::UP:
        case Direction::NONE:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy++;
            }
            outx++;
          }
          break;
        case Direction::DOWN:
          outx = gridWidth_ - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = gridHeight_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outy = gridHeight_ - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outx++;
            }
            outy--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = gridWidth_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
                auto object = objectIt.second;
                int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
                observation.get()[idx] = 1;
              }
              outx--;
            }
            outy++;
          }
          break;
      }

    } else {
      auto pGrid = getAvatarObservableGrid(avatarLocation, Direction::NONE);

      uint32_t outx = 0, outy = 0;
      for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
        outy = 0;
        for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
          // place a 1 in every object "slice" where that object appears
          for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
            auto object = objectIt.second;
            int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
            observation.get()[idx] = 1;
          }
          outy++;
        }
        outx++;
      }
    }
  } else {
    // Can optimize these by only updating states that change and keeping a buffer of the entire state
    auto left = observerConfig_.gridXOffset;
    auto right = observerConfig_.gridXOffset + gridWidth_ - 1;
    auto bottom = observerConfig_.gridYOffset;
    auto top = observerConfig_.gridYOffset + gridHeight_ - 1;
    uint32_t outx = 0, outy = 0;
    for (auto objx = left; objx <= right; objx++) {
      outy = 0;
      for (auto objy = bottom; objy <= top; objy++) {
        for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
          auto object = objectIt.second;

          int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
          observation.get()[idx] = 1;
        }
        outy++;
      }
      outx++;
    }
  }

  return observation;
}

void VectorObserver::print(std::shared_ptr<uint8_t> observation) {
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