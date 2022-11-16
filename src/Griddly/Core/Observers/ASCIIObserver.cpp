#include "ASCIIObserver.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace griddly {

ASCIIObserver::ASCIIObserver(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Observer>> playerObservers) : Observer(std::move(grid), std::move(playerObservers)) {}

void ASCIIObserver::init(ASCIIObserverConfig& config) {
  Observer::init(config);
  config_ = config;
}

void ASCIIObserver::reset() {
  Observer::reset();

  // there are no additional steps until this observer can be used.
  observerState_ = ObserverState::READY;
}

ObserverType ASCIIObserver::getObserverType() const {
  return ObserverType::ASCII;
}

void ASCIIObserver::resetShape() {
  gridWidth_ = config_.overrideGridWidth > 0 ? config_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config_.overrideGridHeight > 0 ? config_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  observationChannels_ = config_.asciiPadWidth;

  observationShape_ = {observationChannels_, gridWidth_, gridHeight_};
  observationStrides_ = {1, observationChannels_, observationChannels_ * gridWidth_};

  size_t obsBufferSize = observationChannels_ * gridWidth_ * gridHeight_;

  observation_ = std::shared_ptr<uint8_t>(new uint8_t[obsBufferSize]);
  memset(observation_.get(), ' ', obsBufferSize);
  for (int x = 0; x < obsBufferSize; x += observationChannels_) {
    *(observation_.get() + x) = '.';
  }

}

void ASCIIObserver::renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation) const {
  auto charPtr = observation_.get() + observationChannels_ * (gridWidth_ * outputLocation.y + outputLocation.x);

  if (resetLocation) {
    auto size = sizeof(uint8_t) * observationChannels_;
    memset(charPtr, ' ', size);
  }

  char mapCharacter;
  if (grid_->getObjectsAt(objectLocation).size() > 0) {
    auto objectIt = grid_->getObjectsAt(objectLocation).begin();
    auto object = objectIt->second;
    auto objectName = object->getObjectName();
    mapCharacter = object->getMapCharacter();

    spdlog::debug("Rendering object {0}", objectName);

    charPtr[0] = mapCharacter;
    if (config_.includePlayerId) {
      auto playerIdx = getEgocentricPlayerId(object->getPlayerId());

      if (playerIdx > 0) {
        auto playerIdxString = std::to_string(playerIdx);
        memcpy(charPtr + 1, playerIdxString.c_str(), sizeof(char) * playerIdxString.length());
      }
    }

  } else {
    charPtr[0] = '.';
  }
}

uint8_t& ASCIIObserver::update() {
  spdlog::debug("ASCII renderer updating.");

  if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer not ready, must be initialized and reset before update() can be called.");
  }

  if (doTrackAvatar_) {
    spdlog::debug("Tracking Avatar.");

    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarOrientation.getDirection();

    // Have to reset the observation
    auto size = sizeof(uint8_t) * observationChannels_ * gridWidth_ * gridHeight_;
    memset(observation_.get(), ' ', size);

    if (config_.rotateWithAvatar) {
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
              renderLocation({objx, objy}, {outx, outy});
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
              renderLocation({objx, objy}, {outx, outy});
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
              renderLocation({objx, objy}, {outx, outy});
              outx++;
            }
            outy--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = gridWidth_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation({objx, objy}, {outx, outy});
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
            renderLocation({objx, objy}, {outx, outy});
          }
          outy++;
        }
        outx++;
      }
    }
  } else {
    const auto& updatedLocations = grid_->getUpdatedLocations(config_.playerId);

    for (auto& location : updatedLocations) {
      if (location.x >= config_.gridXOffset &&
          location.x < gridWidth_ + config_.gridXOffset &&
          location.y >= config_.gridYOffset &&
          location.y < gridHeight_ + config_.gridYOffset) {
        auto outputLocation = glm::ivec2(
            location.x - config_.gridXOffset,
            location.y - config_.gridYOffset);

        spdlog::debug("Rendering location {0}, {1}.", location.x, location.y);

        if (outputLocation.x < gridWidth_ && outputLocation.x >= 0 && outputLocation.y < gridHeight_ && outputLocation.y >= 0) {
          renderLocation(location, outputLocation, true);
        }
      }
    }
  }

  spdlog::debug("Purging update locations.");

  grid_->purgeUpdatedLocations(config_.playerId);

  spdlog::debug("ASCII renderer done.");

  return *observation_.get();
}

}  // namespace griddly
