#include "DelayedActionQueueItem.hpp"

namespace griddly {

DelayedActionQueueItem::DelayedActionQueueItem(uint32_t _playerId, uint32_t _priority, std::shared_ptr<Action> _action)
    : playerId(_playerId), priority(_priority), action(_action) {
}

}  // namespace griddly