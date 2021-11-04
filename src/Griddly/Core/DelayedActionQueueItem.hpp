#pragma once
#include <memory>

#include "GDY/Actions/Action.hpp"

namespace griddly {

class DelayedActionQueueItem {
 public:
  DelayedActionQueueItem(uint32_t _playerId, uint32_t _priority, std::shared_ptr<Action> _action)
      : playerId(_playerId), priority(_priority), action(_action) {
  }
  bool operator==(const DelayedActionQueueItem& other) const {
    return priority == other.priority;
  }

  bool operator>(const DelayedActionQueueItem& other) const {
    return priority < other.priority;
  }

  bool operator<(const DelayedActionQueueItem& other) const {
    return priority > other.priority;
  }

  uint32_t playerId;
  uint32_t priority;
  std::shared_ptr<Action> action;
};

}  // namespace griddly