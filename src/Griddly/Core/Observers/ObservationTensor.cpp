#include "ObservationTensor.hpp"

namespace griddly {

ObservationTensor::ObservationTensor(std::vector<int64_t> shape, std::vector<int64_t> strides, void* data, DLDevice device, DLDataType dtype, uint64_t offset) {
  tensor_ = std::make_shared<DLManagedTensor>();

  tensor_->dl_tensor.ndim = static_cast<int32_t>(shape.size());

  shape_ = shape;
  strides_ = strides;

  tensor_->dl_tensor.shape = shape_.data();
  tensor_->dl_tensor.strides = strides_.data();
  tensor_->dl_tensor.byte_offset = offset;

  tensor_->dl_tensor.device = device;
  tensor_->dl_tensor.dtype = dtype;

  tensor_->dl_tensor.data = data;
}

DLManagedTensor* ObservationTensor::getDLTensor() {
  return tensor_.get();
}

std::vector<int64_t>& ObservationTensor::getShape() {
  return shape_;
}

std::vector<int64_t>& ObservationTensor::getStrides() {
  return strides_;
}

}  // namespace griddly