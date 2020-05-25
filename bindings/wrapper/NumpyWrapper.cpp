#pragma once
#include <memory>
#include <vector>

namespace griddly {
template <class Scalar>
class NumpyWrapper {
 public:
  NumpyWrapper(std::vector<uint32_t> shape, std::vector<uint32_t> strides, std::unique_ptr<Scalar[]> data)
      : shape_(shape), strides_(strides), data_(std::move(data)) {
  }

  std::vector<uint32_t> getShape() const { return shape_; }
  std::vector<uint32_t> getStrides() const { return strides_; }
  uint32_t getScalarSize() const { return sizeof(Scalar); }
  Scalar* getData() const { return data_.get(); }

 private:
  const std::vector<uint32_t> shape_;
  const std::vector<uint32_t> strides_;
  const std::unique_ptr<Scalar[]> data_;
};
}  // namespace griddly