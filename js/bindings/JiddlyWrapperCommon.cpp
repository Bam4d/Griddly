#pragma once
#include <emscripten/val.h>

#include "../../src/Griddly/Core/Observers/EntityObserver.hpp"


namespace e = emscripten;


inline e::val wrapEntityObservation(griddly::EntityObservations& entityObservations) {
}

inline e::val wrapObservation(std::shared_ptr<griddly::Observer> observer) {
}

inline e::val wrapObservationDescription(std::shared_ptr<griddly::Observer> observer) {
}

