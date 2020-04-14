#include "Unit.hpp"
#include "../../Grid.hpp"

namespace griddy {
Unit::Unit(int maxHealth) : health_(maxHealth), maxHealth_(maxHealth) {
}

Unit::~Unit() {}

uint Unit::getHealth() const {
  return health_;
}

const uint Unit::getMaxHealth() const {
  return maxHealth_;
}

bool Unit::onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) {
    
}

int Unit::onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) {

}

}  // namespace griddy