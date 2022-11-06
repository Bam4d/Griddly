#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include "Griddly/Core/Observers/ObservationTensor.hpp"

const char* const kDlTensorCapsuleName = "dltensor";

namespace py = pybind11;

namespace griddly {

class ObservationTensorWrapper {
 public:
  ObservationTensorWrapper(std::shared_ptr<ObservationTensor>& dlTensor)
      : dlTensor_(dlTensor) {
  }

  py::object toDLPack(py::object stream) {
    return py::reinterpret_steal<py::object>(PyCapsule_New(dlTensor_->getDLTensor(), kDlTensorCapsuleName, [](PyObject* obj) {
      spdlog::debug("trying to delete stuff but being ignored");
      // Just clear any errors here, we do not delete this memory as its managed completely by the observers themselves.
      PyErr_Clear();
    }));
  }

  py::tuple getDeviceInfo() {
    spdlog::debug("Getting device info");
    const auto& dlTensorDevice = dlTensor_->getDLTensor()->dl_tensor.device;
    return py::make_tuple(static_cast<int>(dlTensorDevice.device_type), dlTensorDevice.device_id);
  }

 private:
  const std::shared_ptr<ObservationTensor>& dlTensor_;
};

}  // namespace griddly