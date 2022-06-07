#pragma once
#include <emscripten/val.h>

#include "../../src/Griddly/Core/Observers/EntityObserver.hpp"

namespace e = emscripten;

inline e::val wrapEntityObservation(griddly::EntityObservations& entityObservations) {
  e::val entityObservation = e::val::object();

  e::val js_entityObservationFeatures = e::val::object();
  for (const auto& entityFeaturesIt : entityObservations.observations) {
    const auto& name = entityFeaturesIt.first;
    const auto& entityFeatures = entityFeaturesIt.second;

    e::val js_entityFeatures = e::val::array();
    for (int i = 0; i < entityFeatures.size(); i++) {
      auto feats = entityFeatures[i];
      e::val js_feats = e::val::array();
      for (int j = 0; j < feats.size(); j++) {
        js_feats.call<void>("push", feats[j]);
      }
      js_entityFeatures.call<void>("push", js_feats);
    }

    js_entityObservationFeatures.set(name, js_entityFeatures);
  }

  e::val js_entityObservationIds = e::val::object();
  for (const auto& entityIdsIt : entityObservations.ids) {
    const auto& name = entityIdsIt.first;
    const auto& entityIds = entityIdsIt.second;

    e::val js_entityIds = e::val::array();
    for (int i = 0; i < entityIds.size(); i++) {
      js_entityIds.call<void>("push", entityIds[i]);
    }

    js_entityObservationIds.set(name, js_entityIds);
  }

  e::val js_entityObservationLocations = e::val::object();
  for (const auto& entityLocationsIt : entityObservations.locations) {
    const auto& id = entityLocationsIt.first;
    const auto& entityLocation = entityLocationsIt.second;

    auto js_entityLocation = e::val::array();
    js_entityLocation.call<void>("push", entityLocation[0]);
    js_entityLocation.call<void>("push", entityLocation[1]);

    js_entityObservationLocations.set(id, js_entityLocation);
  }

  entityObservation.set("Features", js_entityObservationFeatures);
  entityObservation.set("Ids", js_entityObservationIds);
  entityObservation.set("Locations", js_entityObservationLocations);

  e::val js_entityActorIds = e::val::object();
  for (const auto& entityActorIdsIt : entityObservations.actorIds) {
    const auto& name = entityActorIdsIt.first;
    const auto& entityActorIds = entityActorIdsIt.second;

    e::val js_entityActorIds = e::val::array();
    for (int i = 0; i < entityActorIds.size(); i++) {
      js_entityActorIds.call<void>("push", entityActorIds[i]);
    }

    js_entityActorIds.set(name, js_entityActorIds);
  }
  entityObservation.set("ActorIds", js_entityActorIds);

  e::val js_entityObservationsMasks = e::val::object();
  for (const auto& actorMasksIt : entityObservations.actorMasks) {
    const auto& name = actorMasksIt.first;
    const auto& actorMasks = actorMasksIt.second;

    js_entityObservationsMasks.set(name, actorMasks);
  }
  js_entityObservationsMasks.set("ActorMasks", js_entityObservationsMasks);

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
