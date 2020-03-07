#include "Unit.hpp"

namespace griddy {
Unit::Unit(int playerId, int initialHealth) : playerId_(playerId), health_(initialHealth) {
}

Unit::~Unit() {}

int Unit::getHealth() const {
  return health_;
}

bool Unit::onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
    
}

int Unit::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {

}

}  // namespace griddy