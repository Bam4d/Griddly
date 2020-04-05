#include "Observer.hpp"

namespace griddy {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(size_t gridWidth, size_t gridHeight) {
}

void Observer::print(std::unique_ptr<uint8_t[]> observation) {
}

Observer::~Observer() {
}

}  // namespace griddy