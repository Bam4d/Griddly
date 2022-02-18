#pragma once

namespace griddly {

enum class ObserverType { NONE,
                          SPRITE_2D,
                          BLOCK_2D,
                          ISOMETRIC,
                          VECTOR,
                          ASCII };

template <class ObservationDataType>
class ObservationInterface {
 public:
  virtual ~ObservationInterface() = default;
  /**
   * The data is returned as a byte array for consistency across observers and
   * interfaces
   */
  virtual ObservationDataType& update() = 0;

  virtual ObserverType getObserverType() const = 0;
};

}  // namespace griddly