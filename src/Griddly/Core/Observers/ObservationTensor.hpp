#pragma once

#include <memory>
#include <vector>

#include "dlpack.h"

namespace griddly {

class ObservationTensor {
 public:
  ObservationTensor(std::vector<int64_t> shape, std::vector<int64_t> strides, void* data, DLDevice device, DLDataType dtype, uint64_t offset = 0);

  DLManagedTensor* getDLTensor();

  std::vector<int64_t>& getShape();

  std::vector<int64_t>& getStrides();

 private:
  std::shared_ptr<DLManagedTensor> tensor_;
  std::vector<int64_t> strides_;
  std::vector<int64_t> shape_;
};

}  // namespace griddly