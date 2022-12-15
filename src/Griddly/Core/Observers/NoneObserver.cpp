#include "NoneObserver.hpp"

#include <memory>

namespace griddly {

NoneObserver::NoneObserver(std::shared_ptr<Grid> grid, ObserverConfig& config) : Observer(std::move(grid), config) {
  emptyObs_ = std::shared_ptr<uint8_t>(new uint8_t[1]{});  // NOLINT

  emptyTensor_ = std::make_shared<ObservationTensor>(ObservationTensor(
      {1, 1, 1},
      {1, 1, 1},
      emptyObs_.get(), {DLDeviceType::kDLCPU, 0}, {DLDataTypeCode::kDLUInt, 8, 1}));
}

std::shared_ptr<ObservationTensor>& NoneObserver::update() {
  return emptyTensor_;
}

std::shared_ptr<ObservationTensor>& NoneObserver::getObservationTensor() {
  return emptyTensor_;
}

void NoneObserver::resetShape() {
}

std::vector<int64_t>& NoneObserver::getShape() {
  return emptyTensor_->getShape();
}

std::vector<int64_t>& NoneObserver::getStrides() {
  return emptyTensor_->getStrides();
}

ObserverType NoneObserver::getObserverType() const {
  return ObserverType::NONE;
}

}  // namespace griddly
