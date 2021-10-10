
namespace griddly {
class AStarPathNode {
 public:
  AStarPathNode(uint32_t score)
      : playerId(playerId), priority(score), action(action) {
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

  const uint32_t score_;
};
}  // namespace griddly