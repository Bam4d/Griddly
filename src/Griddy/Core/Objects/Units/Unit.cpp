#include "Unit.hpp"
#include "../../Grid.hpp"

namespace griddy {
Unit::Unit(int playerId, int maxHealth) : playerId_(playerId), health_(maxHealth), maxHealth_(maxHealth) {
}

Unit::~Unit() {}

uint8_t Unit::getHealth() const {
  return health_;
}

const uint8_t Unit::getMaxHealth() const {
  return maxHealth_;
}

bool Unit::onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
    
}

int Unit::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {

}

}  // namespace griddy