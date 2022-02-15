#pragma once
#include <memory>
#include <queue>
#include "Util/util.hpp"

namespace griddly {

class Action;

class DelayedActionQueueItem {
 public:
  DelayedActionQueueItem(uint32_t _playerId, uint32_t _priority, std::shared_ptr<Action> _action);

  uint32_t playerId;
  uint32_t priority;
  std::shared_ptr<Action> action;
};

struct SortDelayedActionQueue {
  bool operator()(const std::shared_ptr<DelayedActionQueueItem>& a, const std::shared_ptr<DelayedActionQueueItem>& b) {
    return a->priority > b->priority;
  };
};

typedef VectorPriorityQueue<std::shared_ptr<DelayedActionQueueItem>, std::vector<std::shared_ptr<DelayedActionQueueItem>>, SortDelayedActionQueue> DelayedActionQueue;

}  // namespace griddly