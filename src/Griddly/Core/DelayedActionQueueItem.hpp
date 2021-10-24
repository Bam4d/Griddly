#pragma once
#include <memory>

#include "GDY/Actions/Action.hpp"

namespace griddly {

class DelayedActionQueueItem {
 public:
  DelayedActionQueueItem(uint32_t playerId, uint32_t priority, std::shared_ptr<Action> action)
      : playerId(playerId), priority(priority), action(action) {
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