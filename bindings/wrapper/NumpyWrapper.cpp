#pragma once
#include <memory>
#include <vector>

namespace griddy {
template <class Scalar>
class NumpyWrapper {
 public:
  NumpyWrapper(std::vector<uint> shape, std::vector<uint> strides, std::unique_ptr<Scalar[]> data)
      : shape_(shape), strides_(strides), data_(std::move(data)) {
  }

  std::vector<uint> getShape() const { return shape_; }
  std::vector<uint> getStrides() const { return strides_; }
  uint getScalarSize() const { return sizeof(Scalar); }
  Scalar* getData() const { return data_.get(); }

 private:
  const std::vector<uint> shape_;
  const std::vector<uint> strides_;
  const std::unique_ptr<Scalar[]> data_;
};
}  // namespace griddy