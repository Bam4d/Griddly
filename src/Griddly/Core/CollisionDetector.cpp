#include "CollisionDetector.hpp"

namespace griddly {

CollisionDetector::CollisionDetector(uint32_t gridWidth, uint32_t gridHeight, uint32_t range)
    : range_(range), gridWidth_(gridWidth), gridHeight_(gridHeight) {
}

}  // namespace griddly