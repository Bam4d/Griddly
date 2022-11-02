#include "NoneObserver.hpp"

#include <memory>

namespace griddly {

NoneObserver::NoneObserver(std::shared_ptr<Grid> grid) : Observer(grid) {
  emptyTensor_.ndim = 3;
  emptyTensorShape_ = {1, 1, 1};
  emptyTensor_.shape = emptyTensorShape_.data();

  emptyTensorStrides_ = {1, 1, 1};
  emptyTensor_.strides = emptyTensorStrides_.data();

  emptyTensor_.byte_offset = 0;
  emptyTensor_.device = {DLDeviceType::kDLCPU, 0};
  emptyTensor_.dtype = {
      DLDataTypeCode::kDLUInt,
      8,
      1};

  emptyObs_ = std::shared_ptr<uint8_t>(new uint8_t[1]{});  //NOLINT

  emptyTensor_.data = emptyObs_.get();
}

const DLTensor& NoneObserver::update() {
  return emptyTensor_;
}

const DLTensor& NoneObserver::getObservationTensor() {
  return emptyTensor_;
}

void NoneObserver::resetShape() {
}

ObserverType NoneObserver::getObserverType() const {
  return ObserverType::NONE;
}

}  // namespace griddly