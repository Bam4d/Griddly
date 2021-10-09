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

void VectorObserver::reset() {
  Observer::reset();

  // there are no additional steps until this observer can be used.
  observerState_ = ObserverState::READY;
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

  observationChannels_ = grid_->getObjectIds().size();

  // Always in order objects, player, orientation, variables.
  if (observerConfig_.includePlayerId) {
    channelsBeforePlayerCount_ = observationChannels_;
    observationChannels_ += observerConfig_.playerCount + 1;  // additional one-hot for "no-player"

    spdlog::debug("Adding {0} playerId channels at: {1}", observationChannels_-channelsBeforePlayerCount_, channelsBeforePlayerCount_);
  }

  if (observerConfig_.includeRotation) {
    channelsBeforeRotation_ = observationChannels_;
    observationChannels_ += 4;
    spdlog::debug("Adding {0} rotation channels at: {1}", observationChannels_-channelsBeforeRotation_, channelsBeforeRotation_);
  }

  if (observerConfig_.includeVariables) {
    channelsBeforeVariables_ = observationChannels_;
    observationChannels_ += grid_->getObjectVariableIds().size();
    spdlog::debug("Adding {0} variable channels at: {1}", observationChannels_-channelsBeforeVariables_, channelsBeforeVariables_);
  }

  observationShape_ = {observationChannels_, gridWidth_, gridHeight_};
  observationStrides_ = {1, observationChannels_, observationChannels_ * gridWidth_};

  observation_ = std::shared_ptr<uint8_t>(new uint8_t[observationChannels_ * gridWidth_ * gridHeight_]{});

  trackAvatar_ = avatarObject_ != nullptr;
}

void VectorObserver::renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation) const {
  auto memPtr = observation_.get() + observationChannels_ * (gridWidth_ * outputLocation.y + outputLocation.x);

  if (resetLocation) {
    auto size = sizeof(uint8_t) * observationChannels_;
    memset(memPtr, 0, size);
  }

  // Only put the *include* information of the first object
  bool processTopLayer = true;
  for (auto& objectIt : grid_->getObjectsAt(objectLocation)) {
    auto object = objectIt.second;
    auto objectName = object->getObjectName();
    spdlog::debug("Rendering object {0}", objectName);
    auto memPtrObject = memPtr + grid_->getObjectIds().at(objectName);
    *memPtrObject = 1;

    if (processTopLayer) {
      if (observerConfig_.includePlayerId) {
        // if we are including the player ID, we always set player = 1 from the perspective of the agent being controlled.
        // e.g if this is observer is owned by player 3 then objects owned by player 3 will be rendered as "player 1".
        // This is so multi-agent games always see the agents they are controlling from first person perspective
        uint32_t playerIdx = 0;
        uint32_t objectPlayerId = object->getPlayerId();

        if (objectPlayerId == 0 || observerConfig_.playerId == 0) {
          playerIdx = objectPlayerId;
        } else if (objectPlayerId < observerConfig_.playerId) {
          playerIdx = objectPlayerId + 1;
        } else if (objectPlayerId == observerConfig_.playerId) {
          playerIdx = 1;
        } else {
          playerIdx = objectPlayerId;
        }

        auto playerMemPtr = memPtr + channelsBeforePlayerCount_ + playerIdx;
        *playerMemPtr = 1;
      }

      if (observerConfig_.includeRotation) {
        uint32_t directionIdx = 0;
        switch (object->getObjectOrientation().getDirection()) {
          case Direction::UP:
          case Direction::NONE:
            directionIdx = 0;
          case Direction::RIGHT:
            directionIdx = 1;
          case Direction::DOWN:
            directionIdx = 2;
          case Direction::LEFT:
            directionIdx = 3;
        }
        auto orientationMemPtr = memPtr + channelsBeforeRotation_ + directionIdx;
        *orientationMemPtr = 1;
      }

      if (observerConfig_.includeVariables) {
        for (auto& variableIt : object->getAvailableVariables()) {
          auto variableValue = *variableIt.second;
          auto variableName = variableIt.first;

          // If the variable is one of the variables defined in the object, get the index of the variable and set it to the variable's value
          auto objectVariableIt = grid_->getObjectVariableIds().find(variableName);
          if (objectVariableIt != grid_->getObjectVariableIds().end()) {
            uint32_t variableIdx = objectVariableIt->second;

            auto variableMemPtr = memPtr + channelsBeforeVariables_ + variableIdx;
            *variableMemPtr = variableValue;

          } 
        }
      }

      processTopLayer = false;
    }
  }
}

uint8_t* VectorObserver::update() {
  spdlog::debug("Vector renderer updating.");
  
  if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer not ready, must be initialized and reset before update() can be called.");
  }

  if (trackAvatar_) {

    spdlog::debug("Tracking Avatar.");

    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarOrientation.getDirection();

    // Have to reset the observation
    auto size = sizeof(uint8_t) * observationChannels_ * gridWidth_ * gridHeight_;
    memset(observation_.get(), 0, size);

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
    const auto& updatedLocations = grid_->getUpdatedLocations(observerConfig_.playerId);

    for (auto& location : updatedLocations) {
      if (location.x >= observerConfig_.gridXOffset &&
          location.x < gridWidth_ + observerConfig_.gridXOffset &&
          location.y >= observerConfig_.gridYOffset &&
          location.y < gridHeight_ + observerConfig_.gridYOffset) {
        auto outputLocation = glm::ivec2(
            location.x - observerConfig_.gridXOffset,
            location.y - observerConfig_.gridYOffset);

        spdlog::debug("Rendering location {0}, {1}.", location.x, location.y);

        if (outputLocation.x < gridWidth_ && outputLocation.x >= 0 && outputLocation.y < gridHeight_ && outputLocation.y >= 0) {
          renderLocation(location, outputLocation, true);
        }
      }
    }
  }

  spdlog::debug("Purging update locations.");

  grid_->purgeUpdatedLocations(observerConfig_.playerId);

  spdlog::debug("Vector renderer done.");

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