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
  return glm::ivec2{1, 1};
}

void VectorObserver::resetShape() {
  gridWidth_ = observerConfig_.overrideGridWidth > 0 ? observerConfig_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = observerConfig_.overrideGridHeight > 0 ? observerConfig_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  auto uniqueObjectCount = grid_->getUniqueObjectCount();

  observationShape_ = {uniqueObjectCount, gridWidth_, gridHeight_};
  observationStrides_ = {1, uniqueObjectCount, uniqueObjectCount * gridWidth_};

  observation_ = std::shared_ptr<uint8_t>(new uint8_t[uniqueObjectCount * gridWidth_ * gridHeight_]{});
}

uint8_t* VectorObserver::reset() {
  resetShape();
  return update();
};

uint8_t* VectorObserver::update() const {
  auto uniqueObjectCount = grid_->getUniqueObjectCount();

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
                observation_.get()[idx] = 1;
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
                observation_.get()[idx] = 1;
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
                observation_.get()[idx] = 1;
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
                observation_.get()[idx] = 1;
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
          if (objx < gridBoundary_.x && objx >= 0 && objy < gridBoundary_.y && objy >= 0) {
            // place a 1 in every object "slice" where that object appears
            for (auto objectIt : grid_->getObjectsAt({objx, objy})) {
              auto object = objectIt.second;
              int idx = uniqueObjectCount * (gridWidth_ * outy + outx) + object->getObjectId();
              observation_.get()[idx] = 1;
            }
          }
          outy++;
        }
        outx++;
      }
    }
  } else {
    const auto& updatedLocations = grid_->getUpdatedLocations(observerConfig_.playerId);

    for (auto& location : updatedLocations) {
      if (location.x >= observerConfig_.gridXOffset &&
          location.x < gridWidth_ + observerConfig_.gridXOffset &&
          location.y >= observerConfig_.gridYOffset &&
          location.y < gridHeight_ + observerConfig_.gridYOffset) {
        auto outputLocation = glm::ivec2(
            location.x - observerConfig_.gridXOffset,
            location.y - observerConfig_.gridYOffset);

        if (outputLocation.x < gridWidth_ && outputLocation.x >= 0 && outputLocation.y < gridHeight_ && outputLocation.y >= 0) {
          auto memPtr = observation_.get() + uniqueObjectCount * (gridWidth_ * outputLocation.y + outputLocation.x);

          auto size = sizeof(uint8_t) * uniqueObjectCount;
          memset(memPtr, 0, size);

          auto& objects = grid_->getObjectsAt(location);
          for (auto objectIt : objects) {
            auto object = objectIt.second;
            auto memPtrObject = memPtr + object->getObjectId();
            *memPtrObject = 1;
          }
        }
      }
    }
  }

  grid_->purgeUpdatedLocations(observerConfig_.playerId);

  return observation_.get();
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