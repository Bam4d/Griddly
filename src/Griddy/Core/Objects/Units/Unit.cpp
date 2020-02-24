#include "Unit.hpp"

namespace griddy {
Unit::Unit(int playerId, int initialHealth) : playerId_(playerId), health_(initialHealth) {
}

Unit::~Unit() {}

int Unit::getHealth() const {
  return health_;
}

bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
    
}

bool onPerformAction(std::shared_ptr<Action> action) {

}

}  // namespace griddy