#include "Observer.hpp"

namespace griddle {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(uint32_t gridWidth, uint32_t gridHeight) {
}

void Observer::print(std::unique_ptr<uint8_t[]> observation) {
}

Observer::~Observer() {
}

}  // namespace griddle