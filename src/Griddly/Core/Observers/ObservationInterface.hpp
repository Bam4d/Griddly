#pragma once

namespace griddly {

template <class ObservationDataType>
class ObservationInterface {
 public:
  virtual ~ObservationInterface() = default;
  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual ObservationDataType& update() = 0;

};

}  // namespace griddly