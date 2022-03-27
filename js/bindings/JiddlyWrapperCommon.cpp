#pragma once
#include <emscripten/val.h>

#include "../../src/Griddly/Core/Observers/EntityObserver.hpp"

namespace e = emscripten;

inline e::val wrapEntityObservation(griddly::EntityObservations& entityObservations) {
  e::val entityObservation = e::val::object();

  e::val entityObservationsObs = e::val::object();
  for (const auto& entityObservation : entityObservations.observations) {
    const auto& name = entityObservation.first;
    const auto& entityObs = entityObservation.second;

    e::val entityVals = e::val::array();
    for (int i = 0; i < entityObs.size(); i++) {
      auto obs = entityObs[i];
      e::val featureVals = e::val::array();
      for (int j = 0; j < obs.size(); j++) {
        featureVals.call<void>("push", obs[j]);
      }
      entityVals.call<void>("push", featureVals);
    }

    entityObservationsObs.set(name, entityVals);
  }

  e::val entityObservationIds = e::val::object();
  for (const auto& entityIds : entityObservations.ids) {
    const auto& name = entityIds.first;
    const auto& entityIds = entityIds.second;

    e::val entityIds = e::val::array();
    for (int i = 0; i < entityIds.size(); i++) {
      entityIds.call<void>("push", entityIds[i]);
    }

    entityObservationIds.set(name, entityIds);
  }

  e::val entityObservationLocations = e::val::object();
  for (const auto& entityLocations : entityObservations.locations) {
    const auto& id = entityLocations.first;
    const auto& location = entityLocations.second;

    auto js_location = e::val::array();
    js_location.call<void>("push", location[0]);
    js_location.call<void>("push", location[1]);

    entityObservationLocations.set(id, js_location);
  }

  entityObservation.set("Entities", entityObservationsObs);
  entityObservation.set("Ids", entityObservationIds);
  entityObservation.set("Locations", entityObservationLocations);

  e::val entityObservationActorIds = e::val::object();
  for (const auto& entityActorIds : entityObservations.actorIds) {
    const auto& name = entityActorIds.first;
    const auto& actorIds = entityActorIds.second;

    e::val actorIds = e::val::array();
    for (int i = 0; i < entityIds.size(); i++) {
      actorIds.call<void>("push", entityIds[i]);
    }

    entityObservationIds.set(name, entityIds);
  }
  entityObservation.set("ActorIds", entityObservationActorIds);

  e::val entityObservationsMasks = e::val::object();
  for (const auto& actorMask : entityObservations.actorMasks) {
    const auto& name = actorMask.first;
    const auto& mask = actorMask.second;

    entityObservationsMasks.set(name, mask);
  }
  entityObservation.set("ActorMasks", entityObservationsMasks);

  return entityObservation;
}

inline e::val wrapObservation(std::shared_ptr<griddly::Observer> observer) {
  if (observer->getObserverType() == griddly::ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<griddly::EntityObserver>(observer);
    auto& observationData = entityObserver->update();
    return wrapEntityObservation(observationData);
  } else {
    auto tensorObserver = std::dynamic_pointer_cast<griddly::TensorObservationInterface>(observer);
    auto& observationData = tensorObserver->update();

    uint32_t totalSize = 1;
    for (int s = 0; s < tensorObserver->getShape().size(); s++) {
      totalSize *= tensorObserver->getShape()[s];
    }

    // TODO: this could be a recursive algorithm for any shape observations
    e::val yVals = e::val::array();
    for (int y = 0; y < totalSize; y += tensorObserver->getStrides()[2]) {
      e::val xVals = e::val::array();
      for (int x = 0; x < tensorObserver->getStrides()[2]; x += tensorObserver->getStrides()[1]) {
        e::val cVals = e::val::array();
        for (int c = 0; c < tensorObserver->getStrides()[1]; c += tensorObserver->getStrides()[0]) {
          cVals.call<void>("push", *(&observationData + c + x + y));
        }
        xVals.call<void>("push", cVals);
      }
      yVals.call<void>("push", xVals);
    }
    return yVals;
  }
}

inline e::val wrapObservationDescription(std::shared_ptr<griddly::Observer> observer) {
  e::val observationDescription = e::val::object();
  const auto observerType = observer->getObserverType();
  if (observerType == griddly::ObserverType::ENTITY) {
    auto entityObserver = std::dynamic_pointer_cast<griddly::EntityObserver>(observer);
    observationDescription.set("Features", entityObserver->getEntityFeatures());
  } else {
    observationDescription.set("Shape", std::dynamic_pointer_cast<griddly::TensorObservationInterface>(observer)->getShape());
  }

  return observationDescription;
}
