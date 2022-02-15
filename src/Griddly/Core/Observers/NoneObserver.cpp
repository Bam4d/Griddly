#include "NoneObserver.hpp"

#include <memory>

namespace griddly {

NoneObserver::NoneObserver(std::shared_ptr<Grid> grid) : Observer(grid) {
  observationShape_ = {1,1,1};
  observationStrides_ = {1,1,1};

  emptyObs_ = std::make_shared<uint8_t>(uint8_t[1]{});
}

uint8_t* NoneObserver::update() {
  return emptyObs_.get();
}

void NoneObserver::resetShape() {
}

ObserverType NoneObserver::getObserverType() const {
  return ObserverType::NONE;
}

}  // namespace griddly