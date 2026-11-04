#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include "Griddly/Core/GameProcess.hpp"
#include "Griddly/Core/Observers/EntityObserver.hpp"
#include "Griddly/Core/Observers/SDL/SDLObserver.hpp"
#include "NumpyWrapper.cpp"

namespace py = nanobind;

namespace griddly {

inline py::dict wrapEntityObservation(const EntityObservations& entityObservations) {
  py::dict entityObservation;

  py::dict entityObservationsObs;

  for (const auto& entityObservation : entityObservations.observations) {
    const auto& name = entityObservation.first;
    const auto& obs = entityObservation.second;

    entityObservationsObs[name.c_str()] = py::cast(obs);
  }

  entityObservation["Entities"] = entityObservationsObs;
  entityObservation["Ids"] = entityObservations.ids;
  entityObservation["Locations"] = entityObservations.locations;

  entityObservation["ActorIds"] = entityObservations.actorIds;

  py::dict entityObservationsMasks;
  for (const auto& actorMask : entityObservations.actorMasks) {
    const auto& name = actorMask.first;
    const auto& mask = actorMask.second;

    entityObservationsMasks[name.c_str()] = py::cast(mask);
  }
  entityObservation["ActorMasks"] = entityObservationsMasks;

  return entityObservation;
}

// Type-erased result of an observer update, produced without the GIL.
struct RawObservation {
  bool isEntity = false;
  const EntityObservations* entityData = nullptr;
  uint8_t* tensorData = nullptr;
  std::vector<uint32_t> shape;
  std::vector<uint32_t> strides;
};

// Pure C++ observer update. Call with the GIL released.
inline RawObservation updateObservationRaw(std::shared_ptr<Observer> observer) {
  RawObservation raw;
  if (observer->getObserverType() == ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<EntityObserver>(observer);
    raw.isEntity = true;
    raw.entityData = &entityObserver->update();
  } else {
    auto tensorObserver = std::dynamic_pointer_cast<TensorObservationInterface>(observer);
    raw.isEntity = false;
    raw.shape = tensorObserver->getShape();
    raw.strides = tensorObserver->getStrides();
    raw.tensorData = &tensorObserver->update();
  }
  return raw;
}

// Build the Python observation object from a raw update. Call with the GIL held.
inline py::object wrapObservationRaw(const RawObservation& raw) {
  if (raw.isEntity) {
    return wrapEntityObservation(*raw.entityData);
  }
  return py::cast(std::make_shared<NumpyWrapper<uint8_t>>(NumpyWrapper<uint8_t>(raw.shape, raw.strides, *raw.tensorData)));
}

inline py::object wrapObservation(std::shared_ptr<Observer> observer) {
  RawObservation raw;
  {
    // Release the GIL while the observer renders/updates so
    // asyncio/other Python threads are not blocked.
    py::gil_scoped_release release;
    raw = updateObservationRaw(observer);
  }
  return wrapObservationRaw(raw);
}

inline py::dict buildActionResultInfo(const ActionResult& actionResult) {
  py::dict py_info;

  if (actionResult.terminated) {
    py::dict py_playerResults;

    for (auto playerRes : actionResult.playerStates) {
      std::string playerStatusString;
      switch (playerRes.second) {
        case TerminationState::WIN:
          playerStatusString = "Win";
          break;
        case TerminationState::LOSE:
          playerStatusString = "Lose";
          break;
        case TerminationState::NONE:
          playerStatusString = "";
          break;
      }

      if (playerStatusString.size() > 0) {
        py_playerResults[std::to_string(playerRes.first).c_str()] = playerStatusString;
      }
    }
    py_info["PlayerResults"] = py_playerResults;
  }

  return py_info;
}

inline py::object wrapObservationDescription(std::shared_ptr<Observer> observer) {
  py::dict observationDescription;
  const auto observerType = observer->getObserverType();
  if (observerType == ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<EntityObserver>(observer);
    observationDescription["Features"] = entityObserver->getEntityFeatures();
  } else {
    if (observerType == ObserverType::SPRITE_2D || observerType == ObserverType::BLOCK_2D || observerType == ObserverType::ISOMETRIC) {
      auto tileSize = std::dynamic_pointer_cast<SDLObserver>(observer)->getTileSize();
      observationDescription["TileSize"] = py::cast(std::array<uint32_t, 2>{static_cast<uint32_t>(tileSize.x), static_cast<uint32_t>(tileSize.y)});
    }
    observationDescription["Shape"] = py::cast(std::dynamic_pointer_cast<TensorObservationInterface>(observer)->getShape());
  }

  return observationDescription;
}

inline py::object wrapActionSpace(std::shared_ptr<Observer> observer) {
  if (observer->getObserverType() == ObserverType::ENTITY) {
  } else {
  }
}

}  // namespace griddly