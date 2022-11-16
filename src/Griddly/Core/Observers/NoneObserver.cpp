#include "NoneObserver.hpp"

#include <memory>

namespace griddly {

NoneObserver::NoneObserver(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Observer>> playerObservers) : Observer(std::move(grid), std::move(playerObservers)) {
  observationShape_ = {1,1,1};
  observationStrides_ = {1,1,1};

  emptyObs_ = std::shared_ptr<uint8_t>(new uint8_t[1]{}); //NOLINT
}

uint8_t& NoneObserver::update() {
  return *emptyObs_.get();
}

void NoneObserver::resetShape() {
}

ObserverType NoneObserver::getObserverType() const {
  return ObserverType::NONE;
}

}  // namespace griddly