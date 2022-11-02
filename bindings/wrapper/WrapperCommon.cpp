#pragma once
#include <pybind11/pybind11.h>

#include "Griddly/Core/Observers/EntityObserver.hpp"
#include "Griddly/Core/Observers/Vulkan/VulkanObserver.hpp"
#include "NumpyWrapper.cpp"

namespace py = pybind11;

namespace griddly {

inline py::dict wrapEntityObservation(EntityObservations& entityObservations) {
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

inline py::object wrapObservation(std::shared_ptr<Observer> observer) {
  if (observer->getObserverType() == ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<EntityObserver>(observer);
    auto& observationData = entityObserver->update();
    return wrapEntityObservation(observationData);
  } else {
    auto tensorObserver = std::dynamic_pointer_cast<TensorObservationInterface>(observer);
    
    return tensorObserver->update();
    //return  py::cast(std::make_shared<NumpyWrapper<uint8_t>>(NumpyWrapper<uint8_t>(tensorObserver->getShape(), tensorObserver->getStrides(), observationData)));
  }
}

inline py::object wrapObservationDescription(std::shared_ptr<Observer> observer) {
  py::dict observationDescription;
  const auto observerType = observer->getObserverType();
  if (observerType == ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<EntityObserver>(observer);
    observationDescription["Features"] = entityObserver->getEntityFeatures();
  } else {
    if (observerType == ObserverType::SPRITE_2D || observerType == ObserverType::BLOCK_2D || observerType == ObserverType::ISOMETRIC) {
      auto tileSize = std::dynamic_pointer_cast<VulkanObserver>(observer)->getTileSize();
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