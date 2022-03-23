#pragma once
#include <memory>
#include <vector>

namespace griddly {
template <class Scalar>
class NumpyWrapper {
 public:
  NumpyWrapper(const std::vector<uint32_t> shape, const std::vector<uint32_t> strides, Scalar& data)
      : shape_(shape), strides_(strides), data_(data) {
  }

  std::vector<uint32_t> getShape() const { return shape_; }
  std::vector<uint32_t> getStrides() const { return strides_; }
  uint32_t getScalarSize() const { return sizeof(Scalar); }
  Scalar& getData() const { return data_; }

 private:
  const std::vector<uint32_t> shape_;
  const std::vector<uint32_t> strides_;
  Scalar& data_;
};
}  // namespace griddly