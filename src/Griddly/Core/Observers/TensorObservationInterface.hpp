#pragma once

#include <vector>

#include "ObservationInterface.hpp"
#include "dlpack.h"

namespace griddly {

class TensorObservationInterface : public ObservationInterface<DLTensor> {
 public:
  virtual ~TensorObservationInterface() = default;

  virtual const DLTensor& getObservationTensor() = 0;

};

}  // namespace griddly
