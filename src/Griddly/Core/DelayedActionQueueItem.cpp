#include "DelayedActionQueueItem.hpp"

#include <utility>

#include <utility>

namespace griddly {

DelayedActionQueueItem::DelayedActionQueueItem(uint32_t _playerId, uint32_t _priority, std::shared_ptr<Action> _action)
    : playerId(_playerId), priority(_priority), action(std::move(std::move(_action))) {
}

}  // namespace griddly