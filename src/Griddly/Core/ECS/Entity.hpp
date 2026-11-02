#pragma once

#include <cstdint>

namespace griddly {

// Stable entity handle: slot index + generation. When a slot is recycled the
// generation is bumped, so stale EntityIds can never alias a live entity.
struct EntityId {
  uint32_t index = 0;
  uint32_t generation = 0;

  bool operator==(const EntityId& other) const {
    return index == other.index && generation == other.generation;
  }
  bool operator!=(const EntityId& other) const {
    return !(*this == other);
  }
};

constexpr EntityId NULL_ENTITY{0xFFFFFFFFu, 0u};

}  // namespace griddly
