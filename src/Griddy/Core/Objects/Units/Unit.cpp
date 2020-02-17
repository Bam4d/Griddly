#include "Unit.hpp"

namespace griddy {
    Unit::Unit(int playerId, int initialHealth) : playerId_(playerId), health_(initialHealth) {

    }

    Unit::~Unit() {}

    int Unit::getHealth() const {
        return health_;
    }
}