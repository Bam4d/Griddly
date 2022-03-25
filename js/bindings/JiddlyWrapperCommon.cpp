#pragma once
#include <emscripten/val.h>

#include "../../src/Griddly/Core/Observers/EntityObserver.hpp"


namespace e = emscripten;


inline e::val::object wrapEntityObservation(EntityObservations& entityObservations) {
}

inline e::val::object wrapObservation(std::shared_ptr<Observer> observer) {
}

inline e::val::object wrapObservationDescription(std::shared_ptr<Observer> observer) {
}

}
