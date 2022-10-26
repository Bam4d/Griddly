#pragma once

#include <vector>

#include "ObservationInterface.hpp"

namespace griddly {

class TensorObservationInterface : public ObservationInterface<uint8_t> {
 public:
  virtual ~TensorObservationInterface() = default;

  virtual std::vector<uint32_t> getShape() const {
    return observationShape_;
  }

  virtual std::vector<uint32_t> getStrides() const {
    return observationStrides_;
  }

 protected:
  std::vector<uint32_t> observationShape_{};
  std::vector<uint32_t> observationStrides_{};
};

}  // namespace griddly
