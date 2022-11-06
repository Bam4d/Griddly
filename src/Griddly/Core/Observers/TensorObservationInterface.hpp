#pragma once

#include <memory>
#include <vector>

#include "ObservationInterface.hpp"
#include "ObservationTensor.hpp"

namespace griddly {

class TensorObservationInterface : public ObservationInterface<std::shared_ptr<ObservationTensor>> {
 public:
  virtual ~TensorObservationInterface() = default;

  virtual std::shared_ptr<ObservationTensor>& getObservationTensor() = 0;

  virtual std::vector<int64_t>& getShape() = 0;

  virtual std::vector<int64_t>& getStrides() = 0;
};

}  // namespace griddly
